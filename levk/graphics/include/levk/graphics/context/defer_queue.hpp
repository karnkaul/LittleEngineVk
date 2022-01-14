#pragma once
#include <ktl/async/kfunction.hpp>
#include <ktl/async/kmutex.hpp>
#include <levk/core/std_types.hpp>
#include <levk/graphics/render/buffering.hpp>
#include <vector>

namespace le::graphics {
class DeferQueue : public Pinned {
  public:
	using Callback = ktl::kfunction<void()>;

	inline static Buffering defaultDefer = Buffering::eDouble;

	void defer(Callback&& callback, Buffering defer = defaultDefer);
	std::size_t decrement();
	void flush();

  protected:
	struct Entry {
		Callback callback;
		Buffering defer;
		bool done;
	};

	mutable ktl::strict_tmutex<std::vector<Entry>> m_entries;
};
} // namespace le::graphics
