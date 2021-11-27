#include <core/ref.hpp>
#include <core/span.hpp>
#include <core/utils/algo.hpp>
#include <graphics/context/defer_queue.hpp>
#include <algorithm>

namespace le::graphics {
void DeferQueue::defer(Callback&& callback, Buffering defer) {
	ktl::tlock lock(m_entries);
	lock->push_back({.callback = std::move(callback), .defer = defer == Buffering() ? 1_B : defer, .done = false});
}

std::size_t DeferQueue::decrement() {
	ktl::tlock lock(m_entries);
	std::vector<Ref<Callback const>> done;
	done.reserve(lock->size());
	for (Entry& entry : *lock) {
		if (entry.defer == 0_B) {
			entry.done = true;
			done.push_back(entry.callback);
		} else {
			--entry.defer.value;
		}
	}
	for (Callback const& callback : done) {
		if (callback) { callback(); }
	}
	utils::erase_if(*lock, [](Entry const& d) -> bool { return d.done; });
	return lock->size();
}

void DeferQueue::flush() {
	ktl::tlock lock(m_entries);
	for (auto const& d : *lock) {
		if (d.callback) { d.callback(); }
	}
	lock.get().clear();
}
} // namespace le::graphics
