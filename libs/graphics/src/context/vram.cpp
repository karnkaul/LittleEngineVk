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
	vk::Image image;
	vk::Extent3D extent;
	LayoutPair fromTo;
	u32 layerCount = 1U;
	u32 mipCount = 1U;
	vk::ImageAspectFlags aspects = vIAFB::eColor;

	ImageCopier(VRAM& vram, Imgs&& imgs, Transfer::Promise&& pr, Image const& img, vk::ImageAspectFlags aspects, LayoutPair fromTo)
		: imgs(std::move(imgs)), vram(vram), pr(std::move(pr)), fromTo(fromTo), aspects(aspects) {
		image = img.image();
		layerCount = img.layerCount();
		mipCount = img.mipCount();
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
		if (mipCount > 1U) { makeMipMaps(stage.command); }
		vram.m_transfer.addStage(std::move(stage), std::move(pr));
		vram.m_device->m_layouts.force(image, meta.layouts.second);
	}

	void makeMipMaps(vk::CommandBuffer cb) const {
		AccessPair access;
		StagePair stages = {vPSFB::eTopOfPipe, vPSFB::eBottomOfPipe};
		CommandBuffer cmd(cb, true);
		cmd.transitionImage(image, layerCount, 1U, 0, aspects, {fromTo.first, vIL::eTransferDstOptimal}, {}, stages);
		cmd.transitionImage(image, layerCount, mipCount - 1U, 1U, aspects, {vIL::eUndefined, vIL::eTransferDstOptimal}, {}, stages);
		vk::Extent3D mipExtent = extent;
		u32 mip = 1;
		for (; mip < mipCount; ++mip) {
			access = {vAFB::eTransferWrite, vAFB::eTransferRead};
			stages = {vPSFB::eTransfer, vPSFB::eTransfer};
			vk::Extent3D nextMipExtent = vk::Extent3D(std::max(mipExtent.width / 2, 1U), std::max(mipExtent.height / 2, 1U), 1U);
			cmd.transitionImage(image, layerCount, 1U, mip - 1, aspects, {vIL::eTransferDstOptimal, vIL::eTransferSrcOptimal}, access, stages);
			vk::ImageBlit region{};
			region.srcOffsets[0] = vk::Offset3D{0, 0, 0};
			region.srcOffsets[1] = vk::Offset3D{int(mipExtent.width), int(mipExtent.height), 1};
			region.srcSubresource.aspectMask = aspects;
			region.srcSubresource.mipLevel = mip - 1;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.layerCount = layerCount;
			region.dstOffsets[0] = vk::Offset3D{0, 0, 0};
			region.dstOffsets[1] = vk::Offset3D{int(nextMipExtent.width), int(nextMipExtent.height), 1U};
			region.dstSubresource.aspectMask = aspects;
			region.dstSubresource.mipLevel = mip;
			region.dstSubresource.baseArrayLayer = 0;
			region.dstSubresource.layerCount = layerCount;
			cb.blitImage(image, vIL::eTransferSrcOptimal, image, vIL::eTransferDstOptimal, region, vk::Filter::eLinear);
			access = {vAFB::eTransferWrite, vAFB::eShaderRead};
			stages = {vPSFB::eTransfer, vPSFB::eAllCommands};
			cmd.transitionImage(image, layerCount, 1U, mip - 1, aspects, {vIL::eTransferSrcOptimal, fromTo.second}, access, stages);
			mipExtent = nextMipExtent;
		}
		cmd.transitionImage(image, layerCount, 1U, mip - 1U, aspects, {vIL::eTransferDstOptimal, fromTo.second}, access, stages);
	}
};

VRAM::VRAM(not_null<Device*> device, Transfer::CreateInfo const& transferInfo) : Memory(device), m_device(device), m_transfer(this, transferInfo) {
	logI(LC_LibUser, "[{}] VRAM constructed", g_name);
}

VRAM::~VRAM() { logI(LC_LibUser, "[{}] VRAM destroyed", g_name); }

Buffer VRAM::makeBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, bool hostVisible) {
	Buffer::CreateInfo bufferInfo;
	bufferInfo.size = size;
	bufferInfo.qcaps = QType::eGraphics;
	if (hostVisible) {
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	} else {
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	}
	bufferInfo.usage = usage | vk::BufferUsageFlagBits::eTransferDst;
	return Buffer(this, bufferInfo);
}

VRAM::Future VRAM::stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size) {
	if (size == 0) { size = out_deviceBuffer.writeSize(); }
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

VRAM::Future VRAM::copyAsync(Span<BmpView const> bitmaps, Image& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects) {
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

VRAM::Future VRAM::copyAsync(Images&& imgs, Image& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects) {
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
