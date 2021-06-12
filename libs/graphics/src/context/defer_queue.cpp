#include <algorithm>
#include <core/ref.hpp>
#include <core/span.hpp>
#include <core/utils/algo.hpp>
#include <graphics/context/defer_queue.hpp>

namespace le::graphics {
void DeferQueue::defer(Callback const& callback, Buffering defer) {
	auto lock = m_entries.lock();
	lock.get().push_back({.callback = callback, .defer = defer == Buffering() ? 1_B : defer, .done = false});
}

std::size_t DeferQueue::decrement() {
	auto lock = m_entries.lock();
	std::vector<Ref<Callback const>> done;
	done.reserve(lock.get().size());
	for (Entry& entry : lock.get()) {
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
	utils::erase_if(lock.get(), [](Entry const& d) -> bool { return d.done; });
	return lock.get().size();
}

void DeferQueue::flush() {
	auto lock = m_entries.lock();
	for (auto const& d : lock.get()) {
		if (d.callback) { d.callback(); }
	}
	lock.get().clear();
}
} // namespace le::graphics
