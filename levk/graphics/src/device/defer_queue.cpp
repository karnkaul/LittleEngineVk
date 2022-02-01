#include <levk/core/ref.hpp>
#include <levk/core/span.hpp>
#include <levk/core/utils/algo.hpp>
#include <levk/graphics/device/defer_queue.hpp>
#include <algorithm>

namespace le::graphics {
constexpr Buffering& operator--(Buffering& b) noexcept { return (b = Buffering{u32(b) - 1}, b); }

void DeferQueue::defer(Callback&& callback, Buffering defer) {
	if (callback) { ktl::klock(m_entries)->push_back({std::move(callback), defer + Buffering::eSingle}); }
}

std::size_t DeferQueue::decrement() {
	ktl::klock lock(m_entries);
	for (Entry& entry : *lock) {
		if (entry.defer == Buffering::eNone) {
			entry.callback();
			entry.callback = {};
		} else {
			--entry.defer;
		}
	}
	utils::erase_if(*lock, [](Entry const& d) -> bool { return !d.callback; });
	return lock->size();
}

void DeferQueue::flush() {
	ktl::klock lock(m_entries);
	for (auto const& d : *lock) {
		if (d.callback) { d.callback(); }
	}
	lock.get().clear();
}
} // namespace le::graphics
