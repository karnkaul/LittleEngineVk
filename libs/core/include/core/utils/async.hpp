#pragma once
#include <algorithm>
#include <ktl/future.hpp>
#include <ktl/tmutex.hpp>

namespace le::utils {
class Async {
  public:
	template <typename F, typename... Args>
	using Ret = std::invoke_result_t<F, Args...>;

	template <typename F, typename... Args>
		requires(!std::is_void_v<Ret<F, Args...>>)
	ktl::future_t<Ret<F, Args...>>
	operator()(F&& f, Args&&... args) {
		ktl::promise_t<Ret<F, Args...>> promise;
		auto ret = promise.get_future();
		auto task = [p = std::move(promise), f = std::forward<F>(f), ... a = std::forward<Args>(args)]() mutable {
			p.set_value(f(std::forward<decltype(a)>(a)...));
		};
		add(std::move(task));
		return ret;
	}

	template <typename F, typename... Args>
		requires(std::is_void_v<Ret<F, Args...>>)
	ktl::future_t<void>
	operator()(F&& f, Args&&... args) {
		ktl::promise_t<void> promise;
		auto ret = promise.get_future();
		auto task = [p = std::move(promise), f = std::forward<F>(f), ... a = std::forward<Args>(args)]() mutable {
			f(std::forward<decltype(a)>(a)...);
			p.set_value();
		};
		add(std::move(task));
		return ret;
	}

  private:
	using storage_t = std::vector<ktl::kthread>;
	ktl::strict_tmutex<storage_t> m_threads;

	template <typename F>
	void add(F&& f) {
		auto lock = ktl::tlock(m_threads);
		std::erase_if(*lock, [](ktl::kthread const& thread) { return !thread.active(); });
		lock->push_back(ktl::kthread(std::forward<F>(f)));
	}
};
} // namespace le::utils
