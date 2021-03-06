#pragma once
#include <functional>
#include <vector>
#include <core/std_types.hpp>
#include <graphics/render/buffering.hpp>
#include <kt/tmutex/tmutex.hpp>

namespace le::graphics {
class DeferQueue {
  public:
	using Callback = std::function<void()>;

	inline static Buffering defaultDefer = 2_B;

	void defer(Callback const& callback, Buffering defer = defaultDefer);
	std::size_t decrement();
	void flush();

  protected:
	struct Entry {
		Callback callback;
		Buffering defer;
		bool done;
	};

	mutable kt::strict_tmutex<std::vector<Entry>> m_entries;
};
} // namespace le::graphics
