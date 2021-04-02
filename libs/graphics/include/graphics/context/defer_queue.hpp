#pragma once
#include <functional>
#include <vector>
#include <core/std_types.hpp>
#include <kt/async_queue/locker.hpp>

namespace le::graphics {
struct Deferred {
	using Callback = std::function<void()>;
	inline static constexpr u64 defaultDefer = 3;

	Callback callback;
	u64 defer = defaultDefer;
};

class DeferQueue {
  public:
	void defer(Deferred deferred);
	void defer(Deferred::Callback const& callback, u64 defer = Deferred::defaultDefer);
	std::size_t decrement();
	void flush();

  protected:
	kt::locker_t<std::vector<Deferred>> m_deferred;
};

// impl

inline void DeferQueue::defer(Deferred deferred) {
	auto lock = m_deferred.lock();
	lock.get().push_back(std::move(deferred));
}
inline void DeferQueue::defer(Deferred::Callback const& callback, u64 defer) {
	auto lock = m_deferred.lock();
	lock.get().push_back({callback, defer});
}
} // namespace le::graphics
