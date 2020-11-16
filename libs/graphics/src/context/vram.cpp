#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>

namespace le::graphics {
VRAM::VRAM(Device& device, Transfer::CreateInfo const& transferInfo) : Memory(device), m_transfer(*this, transferInfo), m_device(device) {
	logD("[{}] VRAM constructed", g_name);
}

VRAM::~VRAM() {
	logD("[{}] VRAM destroyed", g_name);
}

View<Buffer> VRAM::createBO(std::string_view name, vk::DeviceSize size, vk::BufferUsageFlags usage, bool bHostVisible) {
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
		bufferInfo.queueFlags = QType::eGraphics | QType::eTransfer;
	}
	bufferInfo.usage = usage | vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.name = name;
	return construct(bufferInfo);
}

VRAM::Future VRAM::copy(CView<Buffer> src, View<Buffer> dst, vk::DeviceSize size) {
	if (!src || !dst) {
		return {};
	}
	if (size == 0) {
		size = src->writeSize;
	}
	[[maybe_unused]] auto const& sq = src->queueFlags;
	[[maybe_unused]] auto const& dq = dst->queueFlags;
	[[maybe_unused]] bool const bReady = sq.test(QType::eTransfer) && dq.test(QType::eTransfer);
	ENSURE(bReady, "Transfer flag not set!");
	bool const bSizes = dst->writeSize >= size;
	ENSURE(bSizes, "Invalid buffer sizes!");
	if (!bReady) {
		logE("[{}] Source/destination buffers missing QType::eTransfer!", g_name);
		return {};
	}
	if (!bSizes) {
		logE("[{}] Source buffer is larger than destination buffer!", g_name);
		return {};
	}
	Device& d = m_device;
	[[maybe_unused]] auto const indices = d.m_queues.familyIndices(QType::eGraphics | QType::eTransfer);
	if (indices.size() > 1) {
		ENSURE(sq.test() <= 1 || src->mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
		ENSURE(dq.test() <= 1 || dst->mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
	}
	auto promise = Transfer::makePromise();
	auto ret = promise->get_future();
	auto f = [p = std::move(promise), src, dst, size, this]() mutable {
		auto stage = m_transfer.newStage(size);
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		stage.command.begin(beginInfo);
		vk::BufferCopy copyRegion;
		copyRegion.size = size;
		stage.command.copyBuffer(src->buffer, dst->buffer, copyRegion);
		stage.command.end();
		m_transfer.addStage(std::move(stage), std::move(p));
	};
	m_transfer.m_queue.push(std::move(f));
	return ret;
}

VRAM::Future VRAM::stage(View<Buffer> deviceBuffer, void const* pData, vk::DeviceSize size) {
	if (!deviceBuffer) {
		return {};
	}
	if (size == 0) {
		size = deviceBuffer->writeSize;
	}
	Device& d = m_device;
	auto const indices = d.m_queues.familyIndices(QType::eGraphics | QType::eTransfer);
	ENSURE(indices.size() == 1 || deviceBuffer->mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	bool const bQueueFlags = deviceBuffer->queueFlags.test(QType::eTransfer);
	ENSURE(bQueueFlags, "Invalid queue flags!");
	if (!bQueueFlags) {
		logE("[{}] Invalid queue flags on source buffer!", g_name);
		return {};
	}
	auto promise = Transfer::makePromise();
	auto ret = promise->get_future();
	auto f = [p = std::move(promise), deviceBuffer, pData, size, this]() mutable {
		auto stage = m_transfer.newStage(size);
		if (write(*stage.buffer, pData, {0, size})) {
			vk::CommandBufferBeginInfo beginInfo;
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			stage.command.begin(beginInfo);
			vk::BufferCopy copyRegion;
			copyRegion.size = size;
			stage.command.copyBuffer(stage.buffer->buffer, deviceBuffer->buffer, copyRegion);
			stage.command.end();
			m_transfer.addStage(std::move(stage), std::move(p));
		} else {
			logE("[{}] Error staging data!", g_name);
			p->set_value();
		}
	};
	m_transfer.m_queue.push(std::move(f));
	return ret;
}

VRAM::Future VRAM::copy(Span<Span<u8>> pixelsArr, View<Image> dst, LayoutTransition layouts) {
	if (!dst) {
		return {};
	}
	std::size_t imgSize = 0;
	std::size_t layerSize = 0;
	for (auto pixels : pixelsArr) {
		ENSURE(layerSize == 0 || layerSize == pixels.extent, "Invalid image data!");
		layerSize = pixels.extent;
		imgSize += layerSize;
	}
	ENSURE(layerSize > 0 && imgSize > 0, "Invalid image data!");
	Device& d = m_device;
	[[maybe_unused]] auto const indices = d.m_queues.familyIndices(QType::eGraphics | QType::eTransfer);
	ENSURE(indices.size() == 1 || dst->mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	auto promise = Transfer::makePromise();
	auto ret = promise->get_future();
	auto f = [p = std::move(promise), pixelsArr, dst, layouts, imgSize, layerSize, this]() mutable {
		auto stage = m_transfer.newStage(imgSize);
		[[maybe_unused]] bool const bResult = mapMemory(*stage.buffer);
		ENSURE(bResult, "Memory map failed");
		u32 layerIdx = 0;
		u32 const layerCount = (u32)pixelsArr.extent;
		std::vector<vk::BufferImageCopy> copyRegions;
		for (auto pixels : pixelsArr) {
			auto const offset = layerIdx * layerSize;
			void* pStart = (u8*)stage.buffer->pMap + offset;
			std::memcpy(pStart, pixels.data(), pixels.extent);
			vk::BufferImageCopy copyRegion;
			copyRegion.bufferOffset = offset;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;
			copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = (u32)layerIdx;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageOffset = vk::Offset3D(0, 0, 0);
			copyRegion.imageExtent = dst->extent;
			copyRegions.push_back(std::move(copyRegion));
			++layerIdx;
		}
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		stage.command.begin(beginInfo);
		vk::ImageMemoryBarrier barrier;
		barrier.oldLayout = layouts.first;
		barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = dst->image;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = layerCount;
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		using vkstg = vk::PipelineStageFlagBits;
		stage.command.pipelineBarrier(vkstg::eTopOfPipe, vkstg::eTransfer, {}, {}, {}, barrier);
		stage.command.copyBufferToImage(stage.buffer->buffer, dst->image, vk::ImageLayout::eTransferDstOptimal, copyRegions);
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = layouts.second;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = dst->image;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = layerCount;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = {};
		stage.command.pipelineBarrier(vkstg::eTransfer, vkstg::eBottomOfPipe, {}, {}, {}, barrier);
		stage.command.end();
		m_transfer.addStage(std::move(stage), std::move(p));
	};
	m_transfer.m_queue.push(std::move(f));
	return ret;
}

void VRAM::defer(View<Buffer> buffer, u64 defer) {
	if (buffer) {
		Device& d = m_device;
		d.defer([this, b = buffer]() { destroy(b); }, defer);
		*buffer = {};
	}
}

void VRAM::defer(View<Image> image, u64 defer) {
	if (image) {
		Device& d = m_device;
		d.defer([this, i = image]() { destroy(i); }, defer);
		*image = {};
	}
}

void VRAM::waitIdle() {
	while (m_transfer.update() > 0) {
		threads::sleep();
	}
}
} // namespace le::graphics
