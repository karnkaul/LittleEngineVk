#include <algorithm>
#include <core/ref.hpp>
#include <core/span.hpp>
#include <core/utils/algo.hpp>
#include <graphics/context/defer_queue.hpp>

namespace le::graphics {
namespace {
std::vector<Ref<Deferred::Callback>> decrementImpl(std::vector<Deferred>& out_v) {
	std::vector<Ref<Deferred::Callback>> ret;
	for (auto& deferred : out_v) {
		if (deferred.defer > 0_B) { --deferred.defer.value; }
		if (deferred.defer == 0_B) { ret.emplace_back(deferred.callback); }
	}
	return ret;
}

void invokeImpl(Span<Ref<Deferred::Callback> const> callbacks) {
	for (Deferred::Callback const& callback : callbacks) {
		if (callback) { callback(); }
	}
}

void clearImpl(std::vector<Deferred>& out_v) {
	utils::erase_if(out_v, [](Deferred const& d) -> bool { return d.defer == 0_B; });
}
} // namespace

std::size_t DeferQueue::decrement() {
	auto lock = m_deferred.lock();
	auto const done = decrementImpl(lock.get());
	invokeImpl(done);
	clearImpl(lock.get());
	return lock.get().size();
}

void DeferQueue::flush() {
	auto lock = m_deferred.lock();
	for (auto const& [callback, _] : lock.get()) {
		if (callback) { callback(); }
	}
	lock.get().clear();
}
} // namespace le::graphics
