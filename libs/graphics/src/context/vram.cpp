#include <core/log_channel.hpp>
#include <core/utils/expect.hpp>
#include <graphics/command_buffer.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
struct PData {
	void const* ptr;
	std::size_t size;

	PData(bytearray const& bytes) noexcept : ptr(bytes.data()), size(bytes.size()) {}
	PData(utils::STBImg const& img) noexcept : ptr(img.bytes.data()), size(img.bytes.size()) {}
};

template <typename T>
TPair<std::size_t> getLayerImageSize(T const& images) {
	TPair<std::size_t> ret;
	for (auto const& image : images) {
		PData const data(image);
		ENSURE(ret.first == 0 || ret.first == data.size, "Invalid image data!");
		ret.first = data.size;
		ret.second += ret.first;
	}
	ENSURE(ret.first > 0 && ret.second > 0, "Invalid image data!");
	return ret;
}
} // namespace

template <typename T>
struct VRAM::ImageCopier {
	using Imgs = ktl::fixed_vector<T, 6>;

	Imgs imgs;
	VRAM& vram;
	mutable Transfer::Promise pr;
	TPair<std::size_t> layerImageSize;
	vk::Extent3D extent;
	vk::Image image;
	u32 layerCount;
	vk::ImageAspectFlags aspects;
	LayoutPair fromTo;

	ImageCopier(VRAM& vram, Imgs&& imgs, Transfer::Promise&& pr, Image const& img, vk::ImageAspectFlags aspects, LayoutPair fromTo)
		: imgs(std::move(imgs)), vram(vram), pr(std::move(pr)), aspects(aspects), fromTo(fromTo) {
		image = img.image();
		layerCount = img.layerCount();
		extent = img.extent();
		layerImageSize = getLayerImageSize(this->imgs);
	}

	void operator()() const {
		auto stage = vram.m_transfer.newStage(layerImageSize.second);
		[[maybe_unused]] bool const bResult = stage.buffer->map();
		ENSURE(bResult, "Memory map failed");
		u32 layerIdx = 0;
		std::vector<vk::BufferImageCopy> copyRegions;
		for (auto const& img : imgs) {
			auto const offset = layerIdx * layerImageSize.first;
			PData const pd(img);
			std::memcpy((u8*)stage.buffer->mapped() + offset, pd.ptr, pd.size);
			copyRegions.push_back(bufferImageCopy(extent, aspects, offset, (u32)layerIdx++));
		}
		ImgMeta meta;
		meta.layouts = fromTo;
		meta.stages.second = vram.m_post.stages;
		meta.access.second = vram.m_post.access;
		meta.layerCount = layerCount;
		copy(stage.command, stage.buffer->buffer(), image, copyRegions, meta);
		vram.m_transfer.addStage(std::move(stage), std::move(pr));
		vram.m_device->m_layouts.force(image, meta.layouts.second);
	}
};

VRAM::VRAM(not_null<Device*> device, Transfer::CreateInfo const& transferInfo) : Memory(device), m_device(device), m_transfer(this, transferInfo) {
	logI(LC_LibUser, "[{}] VRAM constructed", g_name);
	if (device->queues().queue(QType::eTransfer).flags.test(QType::eGraphics)) {
		m_post.access |= vk::AccessFlagBits::eShaderRead;
		m_post.stages = vk::PipelineStageFlagBits::eFragmentShader;
	}
}

VRAM::~VRAM() { logI(LC_LibUser, "[{}] VRAM destroyed", g_name); }

Buffer VRAM::makeBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, bool hostVisible) {
	Buffer::CreateInfo bufferInfo;
	bufferInfo.size = size;
	if (hostVisible) {
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		bufferInfo.queueFlags = QType::eGraphics;
		bufferInfo.share = vk::SharingMode::eExclusive;
	} else {
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		bufferInfo.queueFlags = QFlags(QType::eGraphics) | QType::eTransfer;
	}
	bufferInfo.usage = usage | vk::BufferUsageFlagBits::eTransferDst;
	return Buffer(this, bufferInfo);
}

VRAM::Future VRAM::copy(Buffer const& src, Buffer& out_dst, vk::DeviceSize size) {
	if (size == 0) { size = src.writeSize(); }
	[[maybe_unused]] auto const& sq = src.m_storage.allocation.queueFlags;
	[[maybe_unused]] auto const& dq = out_dst.m_storage.allocation.queueFlags;
	[[maybe_unused]] bool const bReady = sq.test(QType::eTransfer) && dq.test(QType::eTransfer);
	ENSURE(bReady, "Transfer flag not set!");
	bool const bSizes = out_dst.writeSize() >= size;
	ENSURE(bSizes, "Invalid buffer sizes!");
	if (!bReady) {
		logE(LC_LibUser, "[{}] Source/destination buffers missing QType::eTransfer!", g_name);
		return {};
	}
	if (!bSizes) {
		logE(LC_LibUser, "[{}] Source buffer is larger than destination buffer!", g_name);
		return {};
	}
	[[maybe_unused]] auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::eTransfer);
	if (indices.size() > 1) {
		ENSURE(sq.count() <= 1 || src.m_storage.allocation.mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
		ENSURE(dq.count() <= 1 || out_dst.m_storage.allocation.mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
	}
	Transfer::Promise promise;
	auto ret = promise.get_future();
	auto f = [p = std::move(promise), s = src.buffer(), d = out_dst.buffer(), size, this]() mutable {
		auto stage = m_transfer.newStage(size);
		copy(stage.command, s, d, size);
		m_transfer.addStage(std::move(stage), std::move(p));
	};
	m_transfer.m_queue.push(std::move(f));
	return ret;
}

VRAM::Future VRAM::stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size) {
	if (size == 0) { size = out_deviceBuffer.writeSize(); }
	auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::eTransfer);
	ENSURE(indices.size() == 1 || out_deviceBuffer.m_storage.allocation.mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	bool const bQueueFlags = out_deviceBuffer.m_storage.allocation.queueFlags.test(QType::eTransfer);
	ENSURE(bQueueFlags, "Invalid queue flags!");
	if (!bQueueFlags) {
		logE(LC_LibUser, "[{}] Invalid queue flags on source buffer!", g_name);
		return {};
	}
	bytearray data((std::size_t)size, {});
	std::memcpy(data.data(), pData, data.size());
	Transfer::Promise promise;
	auto ret = promise.get_future();
	auto f = [p = std::move(promise), dst = out_deviceBuffer.buffer(), d = std::move(data), this]() mutable {
		auto stage = m_transfer.newStage(vk::DeviceSize(d.size()));
		if (stage.buffer->write(d.data(), d.size())) {
			copy(stage.command, stage.buffer->buffer(), dst, d.size());
			m_transfer.addStage(std::move(stage), std::move(p));
		} else {
			logE(LC_LibUser, "[{}] Error staging data!", g_name);
			p.set_value();
		}
	};
	m_transfer.m_queue.push(std::move(f));
	return ret;
}

VRAM::Future VRAM::copy(Span<BmpView const> bitmaps, Image& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects) {
	[[maybe_unused]] auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::eTransfer);
	ENSURE(indices.size() == 1 || out_dst.m_storage.allocation.mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	ENSURE((out_dst.usage() & vk::ImageUsageFlagBits::eTransferDst) == vk::ImageUsageFlagBits::eTransferDst, "Transfer bit not set");
	ENSURE(m_device->m_layouts.get(out_dst.image()) == fromTo.first, "Mismatched image layouts");
	Transfer::Promise promise;
	auto ret = promise.get_future();
	ktl::fixed_vector<bytearray, 6> imgs;
	for (auto layer : bitmaps) {
		bytearray bytes(layer.size(), {});
		std::memcpy(bytes.data(), layer.data(), bytes.size());
		imgs.push_back(std::move(bytes));
	}
	out_dst.m_storage.layerCount = (u32)imgs.size();
	m_transfer.m_queue.push(ImageCopier<bytearray>(*this, std::move(imgs), std::move(promise), out_dst, aspects, fromTo));
	return ret;
}

VRAM::Future VRAM::copy(Images&& imgs, Image& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects) {
	[[maybe_unused]] auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::eTransfer);
	ENSURE(indices.size() == 1 || out_dst.m_storage.allocation.mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	ENSURE((out_dst.usage() & vk::ImageUsageFlagBits::eTransferDst) == vk::ImageUsageFlagBits::eTransferDst, "Transfer bit not set");
	ENSURE(m_device->m_layouts.get(out_dst.image()) == fromTo.first, "Mismatched image layouts");
	Transfer::Promise promise;
	auto ret = promise.get_future();
	out_dst.m_storage.layerCount = (u32)imgs.size();
	m_transfer.m_queue.push(ImageCopier<utils::STBImg>(*this, std::move(imgs), std::move(promise), out_dst, aspects, fromTo));
	return ret;
}

bool VRAM::blit(CommandBuffer cb, Image const& src, Image& out_dst, vk::Filter filter, AspectPair aspects) const {
	if ((src.usage() & vk::ImageUsageFlagBits::eTransferSrc) == vk::ImageUsageFlags()) { return false; }
	if ((out_dst.usage() & vk::ImageUsageFlagBits::eTransferDst) == vk::ImageUsageFlags()) { return false; }
	if (!utils::canBlit(src, out_dst)) { return false; }
	blit(cb.m_cb, {src.image(), out_dst.image()}, {src.extent(), out_dst.extent()}, aspects, filter);
	return true;
}

bool VRAM::blit(CommandBuffer cb, TPair<RenderTarget> images, vk::Filter filter, AspectPair aspects) const {
	if (!m_device->physicalDevice().blitCaps(images.first.format).optimal.test(BlitFlag::eSrc)) { return false; }
	if (!m_device->physicalDevice().blitCaps(images.second.format).optimal.test(BlitFlag::eDst)) { return false; }
	vk::Extent3D const srcExt(cast(images.first.extent), 1);
	vk::Extent3D const dstExt(cast(images.second.extent), 1);
	blit(cb.m_cb, {images.first.image, images.second.image}, {srcExt, dstExt}, aspects, filter);
	return true;
}

bool VRAM::copy(CommandBuffer cb, Image const& src, Image& out_dst, vk::ImageAspectFlags aspects) const {
	if ((src.usage() & vk::ImageUsageFlagBits::eTransferSrc) == vk::ImageUsageFlags()) { return false; }
	if ((out_dst.usage() & vk::ImageUsageFlagBits::eTransferDst) == vk::ImageUsageFlags()) { return false; }
	EXPECT(src.extent() == out_dst.extent());
	copy(cb.m_cb, {src.image(), out_dst.image()}, src.extent(), aspects);
	return true;
}

bool VRAM::genMipMaps(CommandBuffer cb, Image& out_img, vk::ImageAspectFlags aspects) {
	if (out_img.m_storage.mipCount > 1U && out_img.m_storage.view && out_img.m_storage.allocatedSize > 0U) {
		cb.transitionImage(out_img.image(), out_img.layerCount(), 1U, 0, aspects, {vIL::eShaderReadOnlyOptimal, vIL::eTransferDstOptimal}, {},
						   {vPSFB::eTopOfPipe, vPSFB::eBottomOfPipe});
		cb.transitionImage(out_img.image(), out_img.layerCount(), out_img.mipCount() - 1U, 1U, aspects, {vIL::eUndefined, vIL::eTransferDstOptimal}, {},
						   {vPSFB::eTopOfPipe, vPSFB::eBottomOfPipe});
		ImgMeta meta;
		meta.layerCount = out_img.layerCount();
		meta.aspects = aspects;
		meta.access = {vAFB::eTransferWrite, vAFB::eTransferRead};
		meta.stages = {vPSFB::eTransfer, vPSFB::eTransfer};
		Extent2D extent = out_img.extent2D();
		u32 mip = 1;
		for (; mip < out_img.m_storage.mipCount; ++mip) {
			cb.transitionImage(out_img.image(), meta.layerCount, 1U, mip - 1, aspects, {vIL::eTransferDstOptimal, vIL::eTransferSrcOptimal}, meta.access,
							   meta.stages);
			vk::ImageBlit region{};
			region.srcOffsets[0] = vk::Offset3D{0, 0, 0};
			region.srcOffsets[1] = vk::Offset3D{int(extent.x), int(extent.y), 1};
			region.srcSubresource.aspectMask = aspects;
			region.srcSubresource.mipLevel = mip - 1;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.layerCount = out_img.layerCount();
			region.dstOffsets[0] = vk::Offset3D{0, 0, 0};
			region.dstOffsets[1] = vk::Offset3D{extent.x > 1 ? int(extent.x / 2) : 1, extent.y > 1 ? int(extent.y / 2) : 1, 1};
			region.dstSubresource.aspectMask = aspects;
			region.dstSubresource.mipLevel = mip;
			region.dstSubresource.baseArrayLayer = 0;
			region.dstSubresource.layerCount = out_img.layerCount();
			cb.m_cb.blitImage(out_img.image(), vIL::eTransferSrcOptimal, out_img.image(), vIL::eTransferDstOptimal, region, vk::Filter::eLinear);
			meta.access = {vAFB::eTransferWrite, vAFB::eShaderRead};
			meta.stages = {vPSFB::eTransfer, vPSFB::eAllCommands};
			cb.transitionImage(out_img.image(), meta.layerCount, 1U, mip - 1, aspects, {vIL::eTransferSrcOptimal, vIL::eShaderReadOnlyOptimal}, meta.access,
							   meta.stages);
			if (extent.x > 1) { extent.x /= 2; }
			if (extent.y > 1) { extent.y /= 2; }
		}
		cb.transitionImage(out_img.image(), meta.layerCount, 1U, mip - 1U, aspects, {vIL::eTransferDstOptimal, vIL::eShaderReadOnlyOptimal}, meta.access,
						   meta.stages);
		if (out_img.m_storage.view) {
			m_device->defer([iv = out_img.m_storage.view, d = m_device]() mutable { d->destroy(iv); });
			out_img.m_storage.view = m_device->makeImageView(out_img.image(), out_img.viewFormat(), aspects, out_img.viewType(), out_img.mipCount());
		}
		return true;
	}
	return false;
}

void VRAM::waitIdle() {
	while (m_transfer.update() > 0) { ktl::kthread::yield(); }
}

bool VRAM::update(bool force) {
	if (!m_transfer.polling() || force) {
		m_transfer.update();
		return true;
	}
	return false;
}

void VRAM::shutdown() {
	logI(LC_Library, "[{}] VRAM shutting down", g_name);
	m_transfer.stopTransfer();
	m_transfer.stopPolling();
}
} // namespace le::graphics
