#include <core/ensure.hpp>
#include <graphics/context/device.hpp>
#include <graphics/render/fence.hpp>

namespace le::graphics {
namespace {
void ready(RenderFence::Fence& fence) noexcept { fence.previous = RenderFence::State::eReady; }
void busy(RenderFence::Fence& fence) noexcept { fence.previous = RenderFence::State::eBusy; }
} // namespace

RenderFence::State RenderFence::Fence::state(vk::Device device) const {
	State ret = previous;
	switch (device.getFenceStatus(*this)) {
	case vk::Result::eSuccess: ret = State::eReady; break;
	case vk::Result::eNotReady: break;
	default: break;
	}
	return ret;
}

RenderFence::RenderFence(not_null<Device*> device, Buffering buffering) : m_device(device) {
	for (u8 i = 0; i < buffering.value; ++i) { m_storage.fences.push_back({m_device->makeFence(true), State::eReady}); }
}

RenderFence& RenderFence::operator=(RenderFence&& rhs) {
	if (&rhs != this) {
		destroy();
		m_storage = std::move(rhs.m_storage);
		m_device = rhs.m_device;
	}
	return *this;
}

RenderFence::~RenderFence() { destroy(); }

void RenderFence::wait() {
	auto& ret = current();
	m_device->waitFor(ret);
	ready(ret);
}

void RenderFence::refresh() {
	for (auto& fence : m_storage.fences) {
		m_device->destroy(fence);
		fence = {m_device->makeFence(true), State::eReady};
	}
}

RenderFence::Fence RenderFence::associate(u32 imageIndex) {
	ENSURE(imageIndex < arraySize(m_storage.ptrs), "Invalid imageIndex");
	auto& curr = current();
	auto ret = std::exchange(m_storage.ptrs[(std::size_t)imageIndex], &curr);
	if (ret) { m_device->waitFor(*ret); }
	busy(curr);
	return ret ? *ret : Fence();
}

kt::result<Swapchain::Acquire> RenderFence::acquire(Swapchain& swapchain, RenderSemaphore rs) {
	if (swapchain.flags().any(Swapchain::Flags(Swapchain::Flag::ePaused) | Swapchain::Flag::eOutOfDate)) { return kt::null_result; }
	auto ret = swapchain.acquireNextImage(rs.draw, drawFence()); // waited for by drawing (external) and associate
	if (ret) { associate(ret->index); }
	return ret;
}

bool RenderFence::present(Swapchain& swapchain, vk::SubmitInfo const& info, RenderSemaphore rs) {
	ENSURE(info.signalSemaphoreCount == 0 || *info.pSignalSemaphores == rs.present, "Semaphore mismatch");
	m_device->queues().submit(QType::eGraphics, info, submitFence(), false);
	if (!swapchain.present(rs.present)) { return false; } // signalled by drawing submit
	next();
	return true;
}

void RenderFence::next() noexcept {
	busy(current());
	m_storage.index = (m_storage.index + 1) % m_storage.fences.size();
}

RenderFence::Fence RenderFence::drawFence() {
	auto& ret = current();
	ENSURE(ret.state(m_device->device()) == State::eReady, "Invalid fence state");
	m_device->resetFence(ret);
	ready(ret);
	return ret;
}

RenderFence::Fence RenderFence::submitFence() {
	auto& ret = current();
	m_device->waitFor(ret);
	m_device->resetFence(ret);
	ready(ret);
	return ret;
}

void RenderFence::destroy() {
	m_device->defer([f = m_storage.fences, d = m_device]() {
		for (auto fence : f) { d->destroy(fence); }
	});
}
} // namespace le::graphics
