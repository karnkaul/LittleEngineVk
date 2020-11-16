#include <algorithm>
#include <core/ref.hpp>
#include <core/span.hpp>
#include <graphics/context/defer_queue.hpp>

namespace le::graphics {
namespace {
std::vector<Ref<Deferred::Callback>> decrementImpl(std::vector<Deferred>& out_v) {
	std::vector<Ref<Deferred::Callback>> ret;
	for (auto& deferred : out_v) {
		if (deferred.defer > 0) {
			--deferred.defer;
		}
		if (deferred.defer == 0) {
			ret.emplace_back(deferred.callback);
		}
	}
	return ret;
}

void invokeImpl(Span<Ref<Deferred::Callback>> callbacks) {
	for (Deferred::Callback const& callback : callbacks) {
		if (callback) {
			callback();
		}
	}
}

void clearImpl(std::vector<Deferred>& out_v) {
	auto iter = std::remove_if(out_v.begin(), out_v.end(), [](Deferred const& d) -> bool { return d.defer == 0; });
	out_v.erase(iter, out_v.end());
}
} // namespace

std::size_t DeferQueue::decrement() {
	auto lock = m_mutex.lock();
	auto const done = decrementImpl(m_deferred);
	invokeImpl(done);
	clearImpl(m_deferred);
	return m_deferred.size();
}

void DeferQueue::flush() {
	auto lock = m_mutex.lock();
	for (auto const& [callback, _] : m_deferred) {
		if (callback) {
			callback();
		}
	}
	m_deferred.clear();
}
} // namespace le::graphics
