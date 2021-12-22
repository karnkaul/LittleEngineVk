#include <core/utils/expect.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/utils/command_pool.hpp>

namespace le::graphics {
static constexpr vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

CommandPool::Cmd CommandPool::Cmd::make(Device* device, vk::CommandBuffer cb) {
	Cmd ret;
	ret.fence = ret.fence.make(device->makeFence(true), device);
	ret.cb = CommandBuffer(cb);
	return ret;
}

CommandPool::CommandPool(not_null<Device*> device, QType qtype, std::size_t batch) : m_device(device), m_qtype(qtype), m_batch((u32)batch) {
	EXPECT(qtype == QType::eGraphics || m_device->queues().hasCompute());
	auto const& queue = qtype == QType::eCompute ? *m_device->queues().compute() : m_device->queues().graphics();
	m_pool = m_pool.make(queue.makeCommandPool(m_device->device(), pool_flags_v), device);
}

CommandPool::Cmd CommandPool::acquire() {
	Cmd ret;
	for (auto& cb : m_cbs) {
		if (m_device->device().getFenceStatus(cb.fence) == vk::Result::eSuccess) {
			std::swap(cb, m_cbs.back());
			ret = std::move(m_cbs.back());
			break;
		}
	}
	if (!ret.cb.m_cb) {
		auto const info = vk::CommandBufferAllocateInfo(m_pool, vk::CommandBufferLevel::ePrimary, m_batch);
		auto cbs = m_device->device().allocateCommandBuffers(info);
		m_cbs.reserve(m_cbs.size() + cbs.size());
		for (auto const cb : cbs) { m_cbs.push_back(Cmd::make(m_device, cb)); }
		ret = std::move(m_cbs.back());
	}
	m_cbs.pop_back();
	m_device->resetFence(ret.fence);
	ret.cb.begin(flags);
	return ret;
}

void CommandPool::release(Cmd&& cmd) {
	cmd.cb.end();
	vk::SubmitInfo si(0U, nullptr, {}, 1U, &cmd.cb.m_cb);
	auto const& queue = m_qtype == QType::eCompute ? *m_device->queues().compute() : m_device->queues().primary();
	queue.submit(si, cmd.fence);
	for (auto const& c : m_cbs) {
		if (c.cb.m_cb == cmd.cb.m_cb) { return; }
	}
	m_cbs.push_back(std::move(cmd));
}
} // namespace le::graphics
