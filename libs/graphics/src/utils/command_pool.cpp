#include <core/utils/expect.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/utils/command_pool.hpp>

namespace le::graphics {
static constexpr vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

FencePool::FencePool(not_null<Device*> device, std::size_t count) : m_device(device) {
	m_free.reserve(count);
	for (std::size_t i = 0; i < count; ++i) { m_free.push_back(makeFence()); }
}

vk::Fence FencePool::next() {
	std::erase_if(m_busy, [this](Defer<vk::Fence>& f) {
		if (!m_device->isBusy(f)) {
			m_free.push_back(std::move(f));
			return true;
		}
		return false;
	});
	if (m_free.empty()) { m_free.push_back(makeFence()); }
	auto ret = std::move(m_free.back());
	m_free.pop_back();
	m_device->resetFence(ret);
	m_busy.push_back(std::move(ret));
	return m_busy.back();
}

CommandPool::CommandPool(not_null<Device*> device, QType qtype, std::size_t batch)
	: m_fencePool(device, 0U), m_device(device), m_qtype(qtype), m_batch((u32)batch) {
	EXPECT(qtype == QType::eGraphics || m_device->queues().hasCompute());
	auto const& queue = qtype == QType::eCompute ? *m_device->queues().compute() : m_device->queues().graphics();
	m_pool = m_pool.make(queue.makeCommandPool(m_device->device(), pool_flags_v), device);
}

CommandBuffer CommandPool::acquire() {
	Cmd cmd;
	for (auto& cb : m_cbs) {
		if (!m_device->isBusy(cb.fence)) {
			std::swap(cb, m_cbs.back());
			cmd = std::move(m_cbs.back());
			break;
		}
	}
	if (!cmd.cb) {
		auto const info = vk::CommandBufferAllocateInfo(m_pool, vk::CommandBufferLevel::ePrimary, m_batch);
		auto cbs = m_device->device().allocateCommandBuffers(info);
		m_cbs.reserve(m_cbs.size() + cbs.size());
		for (auto const cb : cbs) { m_cbs.push_back(Cmd{cb, {}}); }
		cmd = std::move(m_cbs.back());
	}
	m_cbs.pop_back();
	auto ret = CommandBuffer(cmd.cb);
	ret.begin(flags);
	return ret;
}

void CommandPool::release(CommandBuffer&& cb, bool block) {
	EXPECT(!std::any_of(m_cbs.begin(), m_cbs.end(), [c = cb.m_cb](Cmd const& cmd) { return cmd.cb == c; }));
	cb.end();
	Cmd cmd{cb.m_cb, m_fencePool.next()};
	vk::SubmitInfo si(0U, nullptr, {}, 1U, &cb.m_cb);
	auto const& queue = m_qtype == QType::eCompute ? *m_device->queues().compute() : m_device->queues().primary();
	queue.submit(si, cmd.fence);
	m_cbs.push_back(std::move(cmd));
	if (block) { m_device->waitFor(cmd.fence); }
}
} // namespace le::graphics
