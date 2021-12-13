#include <core/utils/expect.hpp>
#include <graphics/utils/command_rotator.hpp>

namespace le::graphics {
CommandRotator::Pool::Cmd CommandRotator::Pool::Cmd::make(Device* device, vk::CommandBuffer cb) {
	Cmd ret;
	ret.fence = {device, device->makeFence(false)};
	ret.cb = cb;
	return ret;
}

CommandRotator::Pool::Pool(not_null<Device*> device, QType qtype, std::size_t batch) : m_device(device), m_batch((u32)batch) {
	EXPECT(qtype == QType::eGraphics || m_device->queues().hasCompute());
	auto const& queue = qtype == QType::eCompute ? *m_device->queues().compute() : m_device->queues().graphics();
	m_pool = {m_device, queue.makeCommandPool(m_device->device(), pool_flags_v)};
}

CommandRotator::Pool::Cmd CommandRotator::Pool::acquire() {
	auto const info = vk::CommandBufferAllocateInfo(m_pool, vk::CommandBufferLevel::ePrimary, m_batch);
	if (m_cbs.empty()) {
		auto cbs = m_device->device().allocateCommandBuffers(info);
		m_cbs.reserve(m_cbs.size() + cbs.size());
		for (auto const cb : cbs) { m_cbs.push_back(Cmd::make(m_device, cb)); }
	}
	auto ret = std::move(m_cbs.back());
	m_cbs.pop_back();
	return ret;
}

void CommandRotator::Pool::release(Cmd&& cmd) {
	for (auto const& c : m_cbs) {
		if (c.cb == cmd.cb) { return; }
	}
	m_device->resetFence(cmd.fence);
	m_cbs.push_back(std::move(cmd));
}

CommandRotator::Instant::~Instant() {
	if (valid()) {
		m_rotator->submit(m_cmd);
		m_rotator->m_device->waitFor(m_cmd.fence);
	}
}

CommandRotator::CommandRotator(not_null<Device*> device, QType qtype) : m_pool(device, qtype), m_qtype(qtype), m_device(device) { m_current = make(); }

CommandRotator::Instant CommandRotator::instant() const { return {this, make()}; }

void CommandRotator::submit() {
	releaseDone();
	submit(m_current);
	m_submitted.push_back(std::move(m_current));
	m_current = make();
}

void CommandRotator::releaseDone() {
	std::erase_if(m_submitted, [this](Cmd& cmd) {
		if (m_device->signalled(vk::Fence(cmd.fence))) {
			m_pool.release({std::move(cmd.fence), cmd.cb.m_cb});
			cmd.promise.set_value();
			return true;
		}
		return false;
	});
}

CommandRotator::Cmd CommandRotator::make() const {
	auto acquire = m_pool.acquire();
	Cmd out;
	out = {CommandBuffer(acquire.cb), std::move(acquire.fence), {}};
	out.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	return out;
}

void CommandRotator::submit(Cmd& out) const {
	out.cb.end();
	vk::SubmitInfo si(0U, nullptr, {}, 1U, &out.cb.m_cb);
	auto const& queue = m_qtype == QType::eCompute ? *m_device->queues().compute() : m_device->queues().primary();
	queue.submit(si, out.fence);
}
} // namespace le::graphics
