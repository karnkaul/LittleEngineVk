#include <core/log_channel.hpp>
#include <core/utils/expect.hpp>
#include <levk/graphics/command_buffer.hpp>
#include <levk/graphics/common.hpp>
#include <levk/graphics/context/device.hpp>
#include <levk/graphics/context/vram.hpp>
#include <levk/graphics/utils/utils.hpp>

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

bytearray toBytearray(BmpView bmp) {
	bytearray ret(bmp.size(), {});
	std::memcpy(ret.data(), bmp.data(), ret.size());
	return ret;
}

vk::ImageLayout doMakeMipMaps(vk::CommandBuffer cb, vk::Image img, vk::Extent3D ex, vk::ImageAspectFlags as, u32 lc, u32 mc, LayoutPair lp) {
	if (lp.second == vIL::eUndefined) { lp.second = vIL::eShaderReadOnlyOptimal; }
	VRAM::ImgMeta pre, post;
	pre.aspects = post.aspects = as;
	pre.layerMip.layer.first = post.layerMip.layer.first = 0U;
	pre.layerMip.layer.count = post.layerMip.layer.count = lc;
	pre.access = post.access = {vAFB::eMemoryRead | vAFB::eMemoryWrite, vAFB::eMemoryRead | vAFB::eMemoryWrite};
	// transition mip[0] from fromTo.first to dst, mip[1]-mip[N] from undefined to dst
	pre.stages = {vPSFB::eAllCommands, vPSFB::eTransfer};
	post.stages = {vPSFB::eTransfer, vPSFB::eTransfer};
	pre.layerMip.mip.count = post.layerMip.mip.first = 1U;
	post.layerMip.mip.count = mc - 1U;
	pre.layouts = {lp.first, vIL::eTransferDstOptimal};
	post.layouts = {vIL::eUndefined, vIL::eTransferDstOptimal};
	VRAM::imageBarrier(cb, img, pre);
	VRAM::imageBarrier(cb, img, post);
	// prepare to blit mip[N-1] to mip[N]...
	pre.access = post.access;
	post.access.second |= vAFB::eShaderRead;
	pre.stages = {vPSFB::eTransfer, vPSFB::eTransfer};
	post.stages = {vPSFB::eTransfer, vPSFB::eAllCommands};
	pre.layerMip.mip.count = post.layerMip.mip.count = 1U;
	pre.layouts = {vIL::eTransferDstOptimal, vIL::eTransferSrcOptimal};
	post.layouts = {vIL::eTransferSrcOptimal, lp.second};
	vk::Extent3D mipExtent = ex;
	u32 mip = 1;
	for (; mip < mc; ++mip) {
		vk::Extent3D nextMipExtent = vk::Extent3D(std::max(mipExtent.width / 2, 1U), std::max(mipExtent.height / 2, 1U), 1U);
		auto const srcOffset = vk::Offset3D{int(mipExtent.width), int(mipExtent.height), 1};
		auto const dstOffset = vk::Offset3D{int(nextMipExtent.width), int(nextMipExtent.height), 1U};
		pre.layerMip.mip.first = post.layerMip.mip.first = mip - 1U;
		auto blitMeta = pre;
		blitMeta.layerMip.mip.first = mip;
		auto const region = VRAM::imageBlit({pre, blitMeta}, {{}, srcOffset}, {{}, dstOffset});
		VRAM::imageBarrier(cb, img, pre); // transition mip[N-1] to dst
		cb.blitImage(img, vIL::eTransferSrcOptimal, img, vIL::eTransferDstOptimal, region, vk::Filter::eLinear);
		VRAM::imageBarrier(cb, img, post); // transition mip[N-1] to fromTo.second
		mipExtent = nextMipExtent;
	}
	post.layerMip.mip.first = mip - 1U;
	post.layouts = {vIL::eTransferDstOptimal, lp.second};
	VRAM::imageBarrier(cb, img, post); // transition mip[N] to fromTo.second
	return lp.second;
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
	glm::ivec2 ioffset{};
	u32 layerCount = 1U;
	u32 mipCount = 1U;
	vk::ImageAspectFlags aspects = vIAFB::eColor;

	ImageCopier(VRAM& vram, Imgs&& imgs, Transfer::Promise&& pr, Image const& img, vk::ImageAspectFlags aspects, LayoutPair fromTo, glm::ivec2 ioffset)
		: imgs(std::move(imgs)), vram(vram), pr(std::move(pr)), fromTo(fromTo), ioffset(ioffset), aspects(aspects) {
		image = img.image();
		layerCount = img.layerCount();
		mipCount = img.mipCount();
		extent = img.extent();
		layerImageSize = getLayerImageSize(this->imgs);
	}

	void operator()() const {
		auto stage = vram.m_transfer.newStage(layerImageSize.second);
		void const* data = stage.buffer->map();
		ENSURE(data, "Memory map failed");
		u32 layerIdx = 0;
		std::vector<vk::BufferImageCopy> copyRegions;
		for (auto const& img : imgs) {
			auto const offset = layerIdx * layerImageSize.first;
			PData const pd(img);
			std::memcpy((u8*)data + offset, pd.ptr, pd.size);
			copyRegions.push_back(bufferImageCopy(extent, aspects, offset, ioffset, (u32)layerIdx++));
		}
		ImgMeta meta;
		meta.layouts = fromTo;
		meta.stages.second = vram.m_post.stages;
		meta.access.second = vram.m_post.access;
		meta.layerMip.layer.count = layerCount;
		copy(stage.command, stage.buffer->buffer(), image, copyRegions, meta);
		if (mipCount > 1U) { meta.layouts.second = doMakeMipMaps(stage.command, image, extent, aspects, layerCount, mipCount, fromTo); }
		vram.m_transfer.addStage(std::move(stage), std::move(pr));
		vram.m_device->m_layouts.force(image, meta.layouts.second);
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

VRAM::Future VRAM::copyAsync(Span<BmpView const> bitmaps, Image const& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects) {
	ENSURE((out_dst.usage() & vk::ImageUsageFlagBits::eTransferDst) == vk::ImageUsageFlagBits::eTransferDst, "Transfer bit not set");
	ENSURE(m_device->m_layouts.get(out_dst.image()) == fromTo.first, "Mismatched image layouts");
	ENSURE(out_dst.layerCount() == bitmaps.size(), "Invalid image");
	Transfer::Promise promise;
	auto ret = promise.get_future();
	ktl::fixed_vector<bytearray, 6> imgs;
	for (auto layer : bitmaps) { imgs.push_back(toBytearray(layer)); }
	m_transfer.m_queue.push(ImageCopier<bytearray>(*this, std::move(imgs), std::move(promise), out_dst, aspects, fromTo, {}));
	return ret;
}

VRAM::Future VRAM::copyAsync(Images&& imgs, Image const& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects) {
	ENSURE((out_dst.usage() & vk::ImageUsageFlagBits::eTransferDst) == vk::ImageUsageFlagBits::eTransferDst, "Transfer bit not set");
	ENSURE(m_device->m_layouts.get(out_dst.image()) == fromTo.first, "Mismatched image layouts");
	ENSURE(out_dst.layerCount() == imgs.size(), "Invalid image");
	Transfer::Promise promise;
	auto ret = promise.get_future();
	m_transfer.m_queue.push(ImageCopier<utils::STBImg>(*this, std::move(imgs), std::move(promise), out_dst, aspects, fromTo, {}));
	return ret;
}

bool VRAM::blit(CommandBuffer cb, TPair<ImageRef> const& images, BlitFilter filter, AspectPair aspects) const {
	if (!m_device->physicalDevice().blitCaps(images.first.format).optimal.test(BlitFlag::eSrc)) { return false; }
	if (!m_device->physicalDevice().blitCaps(images.second.format).optimal.test(BlitFlag::eDst)) { return false; }
	vk::Extent3D const srcExt(cast(images.first.extent), 1);
	vk::Extent3D const dstExt(cast(images.second.extent), 1);
	blit(cb.m_cb, {images.first.image, images.second.image}, {srcExt, dstExt}, aspects, filter);
	return true;
}

bool VRAM::copy(CommandBuffer cb, TPair<ImageRef> const& images, vk::ImageAspectFlags aspects) const {
	auto const& ex = images.first.extent;
	copy(cb.m_cb, {images.first.image, images.second.image}, vk::Extent3D(ex.x, ex.y, 1U), aspects);
	return true;
}

bool VRAM::makeMipMaps(CommandBuffer cb, Image const& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects) const {
	if (out_dst.mipCount() > 1U) {
		auto const layout = doMakeMipMaps(cb.m_cb, out_dst.image(), out_dst.extent(), aspects, out_dst.layerCount(), out_dst.mipCount(), fromTo);
		m_device->m_layouts.force(out_dst.image(), layout);
		return true;
	}
	return false;
}

CommandPool& VRAM::commandPool() {
	auto lock = ktl::klock(m_commandPools);
	if (auto it = lock->find(std::this_thread::get_id()); it != lock->end()) { return it->second; }
	auto [it, _] = lock->emplace(std::this_thread::get_id(), m_device);
	return it->second;
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
