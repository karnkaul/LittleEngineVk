#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/frame_sync.hpp>

namespace le::graphics {
BufferedFrameSync::BufferedFrameSync(Device& device, std::size_t size, u32 secondaryCount) : m_device(device) {
	std::size_t idx = 0;
	ts.resize(size);
	for (auto& s : ts) {
		s.sync.drawReady = device.createSemaphore();
		s.sync.presentReady = device.createSemaphore();
		s.sync.drawing = device.createFence(true);
		s.secondary.resize((std::size_t)secondaryCount);
		for (std::size_t i = 0; i < (std::size_t)secondaryCount + 1; ++i) {
			vk::CommandPoolCreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.queueFamilyIndex = device.queues().familyIndex(QType::eGraphics);
			commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
			FrameSync::Command& c = i == 0 ? s.primary : s.secondary[i - 1];
			c.pool = device.device().createCommandPool(commandPoolCreateInfo);
			c.commandBuffer = CommandBuffer::make(device, c.pool, 1, i > 0).back();
			c.buffer = c.commandBuffer.m_cb;
		}
		s.index = (u32)idx++;
	}
	g_log.log(lvl::info, 1, "[{}] BufferedFrameSync (x{}) constructed", g_name, size);
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

void BufferedFrameSync::swap() {
	next();
}

void BufferedFrameSync::refreshSync() {
	if (!ts.empty()) {
		Device& d = m_device;
		d.defer([&d, sync = ts]() mutable {
			for (auto& s : sync) {
				d.destroy(s.sync.drawing, s.sync.drawReady, s.sync.presentReady);
			}
		});
		for (auto& s : ts) {
			s.sync.drawReady = d.createSemaphore();
			s.sync.presentReady = d.createSemaphore();
			s.sync.drawing = d.createFence(true);
		}
		g_log.log(lvl::info, 1, "[{}] BufferedFrameSync refreshed", g_name);
	}
}

void BufferedFrameSync::destroy() {
	if (!ts.empty()) {
		Device& d = m_device;
		d.defer([&d, sync = ts]() mutable {
			for (auto& s : sync) {
				d.destroy(s.framebuffer, s.sync.drawing, s.sync.drawReady, s.sync.presentReady);
				d.destroy(s.primary.pool);
				for (FrameSync::Command& c : s.secondary) {
					d.destroy(c.pool);
				}
			}
		});
		g_log.log(lvl::info, 1, "[{}] BufferedFrameSync destroyed", g_name);
	}
}
} // namespace le::graphics
