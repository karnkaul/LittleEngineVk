#include <graphics/context/device.hpp>
#include <graphics/context/frame_sync.hpp>

namespace le::graphics {
BufferedFrameSync::BufferedFrameSync(Device& device, std::size_t size, u32 secondaryCount) : m_device(device) {
	std::size_t idx = 0;
	ts.resize(size);
	for (auto& sync : ts) {
		sync.drawReady = device.createSemaphore();
		sync.presentReady = device.createSemaphore();
		sync.drawing = device.createFence(true);
		sync.secondary.resize((std::size_t)secondaryCount);
		for (std::size_t i = 0; i < (std::size_t)secondaryCount + 1; ++i) {
			vk::CommandPoolCreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.queueFamilyIndex = device.m_queues.familyIndex(QType::eGraphics);
			commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
			FrameSync::Command& c = i == 0 ? sync.primary : sync.secondary[i - 1];
			c.pool = device.m_device.createCommandPool(commandPoolCreateInfo);
			c.commandBuffer = CommandBuffer::make(device, c.pool, 1, i > 0).back();
			c.buffer = c.commandBuffer.m_cmd;
		}
		sync.index = (u32)idx++;
	}
	logD("[{}] BufferedFrameSync (x{}) constructed", g_name, size);
}

BufferedFrameSync::BufferedFrameSync(BufferedFrameSync&& rhs) : m_device(rhs.m_device) {
	ts = std::move(rhs.ts);
	total = std::exchange(rhs.total, 0);
	index = std::exchange(rhs.index, 0);
}

BufferedFrameSync& BufferedFrameSync::operator=(BufferedFrameSync&& rhs) {
	if (&rhs != this) {
		destroy();
		m_device = rhs.m_device;
		ts = std::move(rhs.ts);
		total = std::exchange(rhs.total, 0);
		index = std::exchange(rhs.index, 0);
	}
	return *this;
}

BufferedFrameSync::~BufferedFrameSync() {
	destroy();
}

void BufferedFrameSync::refreshSync() {
	if (!ts.empty()) {
		Device& d = m_device;
		d.defer([&d, sync = ts]() mutable {
			for (auto& s : sync) {
				d.destroy(s.drawing, s.drawReady, s.presentReady);
			}
		});
		for (auto& sync : ts) {
			sync.drawReady = d.createSemaphore();
			sync.presentReady = d.createSemaphore();
			sync.drawing = d.createFence(true);
		}
		logD("[{}] BufferedFrameSync refreshed", g_name);
	}
}

void BufferedFrameSync::destroy() {
	if (!ts.empty()) {
		Device& d = m_device;
		d.defer([&d, sync = ts]() mutable {
			for (auto& s : sync) {
				d.destroy(s.framebuffer, s.drawing, s.drawReady, s.presentReady);
				d.destroy(s.primary.pool);
				for (FrameSync::Command& c : s.secondary) {
					d.destroy(c.pool);
				}
			}
		});
		logD("[{}] BufferedFrameSync destroyed", g_name);
	}
}
} // namespace le::graphics
