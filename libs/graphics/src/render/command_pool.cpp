#include <algorithm>
#include <graphics/render/command_pool.hpp>

namespace le::graphics {
CommandPool::CommandPool(not_null<Device*> device, vk::CommandPoolCreateFlags flags, QType qtype) : m_qtype(qtype), m_device(device) {
	m_pool = makeDeferred<vk::CommandPool>(m_device, flags | vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_qtype);
}

void CommandPool::update() {
	for (auto& cmd : m_busy) {
		if (m_device->signalled(cmd.fence.get())) {
			cmd.cb.m_cb.reset({});
			m_free.push_back(std::move(cmd));
		}
	}
	std::erase_if(m_busy, [](Cmd const& cmd) { return !cmd.fence.active(); });
}

CommandPool::Cmd CommandPool::newCmd() const { return {CommandBuffer::make(m_device, m_pool, 1U).front(), makeDeferred<vk::Fence>(m_device, false)}; }

CommandPool::Cmd CommandPool::beginCmd() const {
	Cmd ret;
	if (!m_free.empty()) {
		ret = std::move(m_free.back());
		m_free.pop_back();
	} else {
		ret = newCmd();
	}
	ret.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	return ret;
}

void CommandPool::submit(Cmd cmd, vk::Semaphore wait, vk::PipelineStageFlags stages, vk::Semaphore signal) const {
	cmd.cb.end();
	vk::SubmitInfo info;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cmd.cb.m_cb;
	info.waitSemaphoreCount = wait == vk::Semaphore() ? 0 : 1;
	info.pWaitSemaphores = wait == vk::Semaphore() ? nullptr : &wait;
	info.pWaitDstStageMask = &stages;
	info.signalSemaphoreCount = signal == vk::Semaphore() ? 0 : 1;
	info.pSignalSemaphores = signal == vk::Semaphore() ? nullptr : &signal;
	m_device->queues().submit(m_qtype, info, cmd.fence, true);
	m_busy.push_back(std::move(cmd));
}
} // namespace le::graphics
