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
	m_pool = {m_device, m_device->makeCommandPool(pool_flags_v, qtype)};
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

CommandRotator::CommandRotator(not_null<Device*> device, QType qtype) : m_pool(device, qtype), m_qtype(qtype), m_device(device) { updateCurrent(); }

void CommandRotator::immediate(ktl::move_only_function<void(CommandBuffer const&)>&& func) {
	if (func) {
		func(get());
		auto f = fence();
		submit();
		m_device->waitFor(f);
	}
}

void CommandRotator::submit() {
	releaseDone();
	m_current.cb.end();
	vk::SubmitInfo si(0U, nullptr, {}, 1U, &m_current.cb.m_cb);
	m_device->queues().submit(m_qtype, si, m_current.fence, true);
	m_submitted.push_back(std::move(m_current));
	updateCurrent();
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

void CommandRotator::updateCurrent() {
	auto acquire = m_pool.acquire();
	m_current = {CommandBuffer(acquire.cb), std::move(acquire.fence), {}};
	m_current.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	m_current.promise = {};
}
} // namespace le::graphics
