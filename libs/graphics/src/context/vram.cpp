#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/command_buffer.hpp>

namespace le::graphics {
VRAM::VRAM(not_null<Device*> device, Transfer::CreateInfo const& transferInfo) : Memory(device), m_device(device), m_transfer(this, transferInfo) {
	g_log.log(lvl::info, 1, "[{}] VRAM constructed", g_name);
	if (device->queues().queue(QType::eTransfer).flags.test(QType::eGraphics)) {
		m_post.access = vk::AccessFlagBits::eShaderRead;
		m_post.stages = vk::PipelineStageFlagBits::eFragmentShader;
	}
}

VRAM::~VRAM() { g_log.log(lvl::info, 1, "[{}] VRAM destroyed", g_name); }

Buffer VRAM::makeBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, bool bHostVisible) {
	Buffer::CreateInfo bufferInfo;
	bufferInfo.size = size;
	if (bHostVisible) {
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
	[[maybe_unused]] auto const& sq = src.data().queueFlags;
	[[maybe_unused]] auto const& dq = out_dst.data().queueFlags;
	[[maybe_unused]] bool const bReady = sq.test(QType::eTransfer) && dq.test(QType::eTransfer);
	ensure(bReady, "Transfer flag not set!");
	bool const bSizes = out_dst.writeSize() >= size;
	ensure(bSizes, "Invalid buffer sizes!");
	if (!bReady) {
		g_log.log(lvl::error, 1, "[{}] Source/destination buffers missing QType::eTransfer!", g_name);
		return {};
	}
	if (!bSizes) {
		g_log.log(lvl::error, 1, "[{}] Source buffer is larger than destination buffer!", g_name);
		return {};
	}
	[[maybe_unused]] auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::eTransfer);
	if (indices.size() > 1) {
		ensure(sq.test() <= 1 || src.data().mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
		ensure(dq.test() <= 1 || out_dst.data().mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
	}
	auto promise = Transfer::makePromise();
	auto ret = promise->get_future();
	auto f = [p = std::move(promise), s = src.buffer(), d = out_dst.buffer(), size, this]() mutable {
		auto stage = m_transfer.newStage(size);
		copy(stage.command, s, d, size);
		m_transfer.addStage(std::move(stage), std::move(p));
	};
	m_transfer.m_queue.push(std::move(f));
	return {std::move(ret)};
}

VRAM::Future VRAM::stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size) {
	if (size == 0) { size = out_deviceBuffer.writeSize(); }
	auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::eTransfer);
	ensure(indices.size() == 1 || out_deviceBuffer.data().mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	bool const bQueueFlags = out_deviceBuffer.data().queueFlags.test(QType::eTransfer);
	ensure(bQueueFlags, "Invalid queue flags!");
	if (!bQueueFlags) {
		g_log.log(lvl::error, 1, "[{}] Invalid queue flags on source buffer!", g_name);
		return {};
	}
	bytearray data((std::size_t)size, {});
	std::memcpy(data.data(), pData, data.size());
	auto promise = Transfer::makePromise();
	auto ret = promise->get_future();
	auto f = [p = std::move(promise), dst = out_deviceBuffer.buffer(), d = std::move(data), this]() mutable {
		auto stage = m_transfer.newStage(vk::DeviceSize(d.size()));
		if (stage.buffer->write(d.data(), d.size())) {
			copy(stage.command, stage.buffer->buffer(), dst, d.size());
			m_transfer.addStage(std::move(stage), std::move(p));
		} else {
			g_log.log(lvl::error, 1, "[{}] Error staging data!", g_name);
			p->set_value();
		}
	};
	m_transfer.m_queue.push(std::move(f));
	return {std::move(ret)};
}

VRAM::Future VRAM::copy(Span<ImgView const> bitmaps, Image& out_dst, LayoutPair layouts) {
	std::size_t imgSize = 0;
	std::size_t layerSize = 0;
	for (auto pixels : bitmaps) {
		ensure(layerSize == 0 || layerSize == pixels.size(), "Invalid image data!");
		layerSize = pixels.size();
		imgSize += layerSize;
	}
	ensure(layerSize > 0 && imgSize > 0, "Invalid image data!");
	[[maybe_unused]] auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::eTransfer);
	ensure(indices.size() == 1 || out_dst.data().mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	ensure((out_dst.usage() & vk::ImageUsageFlagBits::eTransferDst) == vk::ImageUsageFlagBits::eTransferDst, "Transfer bit not set");
	ensure(out_dst.layout() == layouts.first, "Mismatched image layouts");
	auto promise = Transfer::makePromise();
	auto ret = promise->get_future();
	std::vector<bytearray> data;
	data.reserve(bitmaps.size());
	for (auto layer : bitmaps) {
		bytearray bytes(layer.size(), {});
		std::memcpy(bytes.data(), layer.data(), bytes.size());
		data.push_back(std::move(bytes));
	}
	out_dst.m_storage.layerCount = (u32)data.size();
	auto f = [p = std::move(promise), d = std::move(data), i = out_dst.image(), l = out_dst.m_storage.layerCount, e = out_dst.m_storage.extent, layouts,
			  imgSize, layerSize, this]() mutable {
		auto stage = m_transfer.newStage(imgSize);
		[[maybe_unused]] bool const bResult = stage.buffer->map();
		ensure(bResult, "Memory map failed");
		u32 layerIdx = 0;
		std::vector<vk::BufferImageCopy> copyRegions;
		for (auto const& pixels : d) {
			auto const offset = layerIdx * layerSize;
			std::memcpy((u8*)stage.buffer->mapped() + offset, pixels.data(), pixels.size());
			copyRegions.push_back(bufferImageCopy(e, vk::ImageAspectFlagBits::eColor, offset, (u32)layerIdx++));
		}
		ImgMeta meta;
		meta.layouts = layouts;
		meta.stages.second = m_post.stages;
		meta.access.second = m_post.access;
		meta.layerCount = l;
		copy(stage.command, stage.buffer->buffer(), i, copyRegions, meta);
		m_transfer.addStage(std::move(stage), std::move(p));
	};
	m_transfer.m_queue.push(std::move(f));
	out_dst.layout(layouts.second);
	return {std::move(ret)};
}

VRAM::Future VRAM::blit(Image const& src, Image& out_dst, LayoutPair layouts, TPair<vk::ImageAspectFlags> aspects, vk::Filter filter) {
	ensure((src.usage() & vk::ImageUsageFlagBits::eTransferDst) == vk::ImageUsageFlagBits::eTransferDst, "Transfer bit not set");
	ensure((out_dst.usage() & vk::ImageUsageFlagBits::eTransferDst) == vk::ImageUsageFlagBits::eTransferDst, "Transfer bit not set");
	auto promise = Transfer::makePromise();
	auto ret = promise->get_future();
	TPair<vk::Extent3D> const extents = {src.extent(), out_dst.extent()};
	auto f = [this, p = std::move(promise), s = src.image(), d = out_dst.image(), extents, layouts, aspects, filter]() mutable {
		auto stage = m_transfer.newStage(0);
		blit(stage.command, {s, d}, extents, layouts, filter, aspects);
		m_transfer.addStage(std::move(stage), std::move(p));
	};
	return {std::move(ret)};
}

void VRAM::blit(CommandBuffer cb, TPair<RenderImage> images, LayoutPair layouts, vk::Filter filter, TPair<vk::ImageAspectFlags> aspects) {
	vk::Extent3D const src(cast(images.first.extent), 1);
	vk::Extent3D const dst(cast(images.second.extent), 1);
	blit(cb.m_cb, {images.first.image, images.second.image}, {src, dst}, layouts, filter, aspects);
}

void VRAM::waitIdle() {
	while (m_transfer.update() > 0) { kt::kthread::yield(); }
}

bool VRAM::update(bool force) {
	if (!m_transfer.polling() || force) {
		m_transfer.update();
		return true;
	}
	return false;
}

void VRAM::shutdown() {
	g_log.log(lvl::debug, 2, "[{}] VRAM shutting down", g_name);
	m_transfer.stopPolling();
	m_transfer.stopTransfer();
}
} // namespace le::graphics
