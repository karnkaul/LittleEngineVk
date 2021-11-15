#pragma once
#include <core/log.hpp>
#include <exception>
#include <type_traits>

namespace le::utils {
///
/// \brief Default exception handler (logs error)
///
struct ExceptionHandler {
	void operator()(std::exception const& e) const { logE("Exception: {}", e.what()); }
};

///
/// \brief Invoke a callable within a try-catch block
/// \returns false if exception was caught
///
template <typename Handler = ExceptionHandler, typename F, typename... Args>
bool safeInvoke(F&& func, Args&&... args) noexcept {
	try {
		func(std::forward<Args>(args)...);
		return true;
	} catch (std::exception const& e) {
		Handler{}(e);
		return false;
	}
}
///
/// \brief Invoke a callable within a try-catch block
/// \returns invocation result or except if exception caught
///
template <typename Handler = ExceptionHandler, typename F, typename... Args>
auto safeInvokeR(F&& func, Args&&... args, std::invoke_result_t<F, Args...> const& except) noexcept {
	try {
		return func(std::forward<Args>(args)...);
	} catch (std::exception const& e) {
		Handler{}(e);
		return except;
	}
}

///
/// \brief Default execution policy
///
struct ExecPolicy {
	///
	/// \brief Invocation return type (can ve void)
	///
	using return_type = int;
	///
	/// \brief Exception handler
	///
	using handler_t = ExceptionHandler;
	///
	/// \brief Exception signal, not required if return_type is void (can be mutable)
	///
	static constexpr return_type exception_v = 100;
};
///
/// \brief Executor, customized via Policy
///
template <typename Policy = ExecPolicy>
class Execute {
  public:
	using policy_t = Policy;
	using return_type = std::conditional_t<std::is_void_v<typename Policy::return_type>, int, typename Policy::return_type>;
	using handler_t = typename Policy::handler_t;

	template <typename F, typename... Args>
		requires(std::is_invocable_r_v<typename Policy::return_type, F, Args...>)
	explicit Execute(F&& func, Args&&... args) noexcept(false) : m_value(run(std::forward<F>(func), std::forward<Args>(args)...)) {}

	constexpr operator return_type() const noexcept { return m_value; }

  private:
	template <typename F, typename... Args>
	static return_type run(F&& func, Args&&... args) noexcept(false) {
		if constexpr (std::is_void_v<typename Policy::return_type>) {
			return safeInvoke<handler_t>(std::forward<F>(func), std::forward<Args>(args)...);
		} else {
			return safeInvokeR<handler_t>(std::forward<F>(func), std::forward<Args>(args)..., Policy::exception_v);
		}
	}

	return_type const m_value;
};
} // namespace le::utils
