#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>

namespace le::graphics {
VRAM::VRAM(not_null<Device*> device, Transfer::CreateInfo const& transferInfo) : Memory(device), m_device(device), m_transfer(this, transferInfo) {
	g_log.log(lvl::info, 1, "[{}] VRAM constructed", g_name);
	if (device->queues().queue(QType::eTransfer).flags.test(QType::eGraphics)) {
		m_post.access = vk::AccessFlagBits::eShaderRead;
		m_post.stages = vk::PipelineStageFlagBits::eFragmentShader;
	}
}

VRAM::~VRAM() {
	g_log.log(lvl::info, 1, "[{}] VRAM destroyed", g_name);
}

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
	if (size == 0) {
		size = src.writeSize();
	}
	[[maybe_unused]] auto const& sq = src.data().queueFlags;
	[[maybe_unused]] auto const& dq = out_dst.data().queueFlags;
	[[maybe_unused]] bool const bReady = sq.test(QType::eTransfer) && dq.test(QType::eTransfer);
	ENSURE(bReady, "Transfer flag not set!");
	bool const bSizes = out_dst.writeSize() >= size;
	ENSURE(bSizes, "Invalid buffer sizes!");
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
		ENSURE(sq.test() <= 1 || src.data().mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
		ENSURE(dq.test() <= 1 || out_dst.data().mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
	}
	auto promise = Transfer::makePromise();
	auto ret = promise->get_future();
	auto f = [p = std::move(promise), s = src.buffer(), d = out_dst.buffer(), size, this]() mutable {
		auto stage = m_transfer.newStage(size);
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		stage.command.begin(beginInfo);
		vk::BufferCopy copyRegion;
		copyRegion.size = size;
		stage.command.copyBuffer(s, d, copyRegion);
		stage.command.end();
		m_transfer.addStage(std::move(stage), std::move(p));
	};
	m_transfer.m_queue.push(std::move(f));
	return {std::move(ret)};
}

VRAM::Future VRAM::stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size) {
	if (size == 0) {
		size = out_deviceBuffer.writeSize();
	}
	auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::eTransfer);
	ENSURE(indices.size() == 1 || out_deviceBuffer.data().mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	bool const bQueueFlags = out_deviceBuffer.data().queueFlags.test(QType::eTransfer);
	ENSURE(bQueueFlags, "Invalid queue flags!");
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
		if (stage.buffer.write(d.data(), d.size())) {
			vk::CommandBufferBeginInfo beginInfo;
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			stage.command.begin(beginInfo);
			vk::BufferCopy copyRegion;
			copyRegion.size = vk::DeviceSize(d.size());
			stage.command.copyBuffer(stage.buffer.buffer(), dst, copyRegion);
			stage.command.end();
			m_transfer.addStage(std::move(stage), std::move(p));
		} else {
			g_log.log(lvl::error, 1, "[{}] Error staging data!", g_name);
			p->set_value();
		}
	};
	m_transfer.m_queue.push(std::move(f));
	return {std::move(ret)};
}

VRAM::Future VRAM::copy(View<View<std::byte>> pixelsArr, Image& out_dst, LayoutPair layouts) {
	std::size_t imgSize = 0;
	std::size_t layerSize = 0;
	for (auto pixels : pixelsArr) {
		ENSURE(layerSize == 0 || layerSize == pixels.size(), "Invalid image data!");
		layerSize = pixels.size();
		imgSize += layerSize;
	}
	ENSURE(layerSize > 0 && imgSize > 0, "Invalid image data!");
	[[maybe_unused]] auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::eTransfer);
	ENSURE(indices.size() == 1 || out_dst.data().mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	auto promise = Transfer::makePromise();
	auto ret = promise->get_future();
	std::vector<bytearray> data;
	data.reserve(pixelsArr.size());
	for (auto layer : pixelsArr) {
		bytearray bytes(layer.size(), {});
		std::memcpy(bytes.data(), layer.data(), bytes.size());
		data.push_back(std::move(bytes));
	}
	out_dst.m_storage.layerCount = (u32)data.size();
	auto f = [p = std::move(promise), d = std::move(data), i = out_dst.image(), l = out_dst.m_storage.layerCount, e = out_dst.m_storage.extent, layouts,
			  imgSize, layerSize, this]() mutable {
		auto stage = m_transfer.newStage(imgSize);
		[[maybe_unused]] bool const bResult = stage.buffer.map();
		ENSURE(bResult, "Memory map failed");
		u32 layerIdx = 0;
		std::vector<vk::BufferImageCopy> copyRegions;
		for (auto const& pixels : d) {
			auto const offset = layerIdx * layerSize;
			void* pStart = (u8*)stage.buffer.mapped() + offset;
			std::memcpy(pStart, pixels.data(), pixels.size());
			vk::BufferImageCopy copyRegion;
			copyRegion.bufferOffset = offset;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;
			copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = (u32)layerIdx;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageOffset = vk::Offset3D(0, 0, 0);
			copyRegion.imageExtent = e;
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
		barrier.image = i;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = l;
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		using vkstg = vk::PipelineStageFlagBits;
		stage.command.pipelineBarrier(vkstg::eTopOfPipe, vkstg::eTransfer, {}, {}, {}, barrier);
		stage.command.copyBufferToImage(stage.buffer.buffer(), i, vk::ImageLayout::eTransferDstOptimal, copyRegions);
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = layouts.second;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = i;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = l;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = m_post.access;
		stage.command.pipelineBarrier(vkstg::eTransfer, vkstg::eBottomOfPipe | m_post.stages, {}, {}, {}, barrier);
		stage.command.end();
		m_transfer.addStage(std::move(stage), std::move(p));
	};
	m_transfer.m_queue.push(std::move(f));
	return {std::move(ret)};
}

void VRAM::waitIdle() {
	while (m_transfer.update() > 0) {
		kt::kthread::yield();
	}
}
} // namespace le::graphics
