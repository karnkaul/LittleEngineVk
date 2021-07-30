#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <ktl/monotonic_map.hpp>

namespace le {
///
/// \brief Wrapper for invocation of multiple registered callbacks
///
template <typename... Args>
class Delegate {
  public:
	using Callback = std::function<void(Args...)>;
	using Storage = ktl::monotonic_map<Callback>;
	using Tk = typename Storage::handle;

	///
	/// \brief Register callback and obtain token
	/// \returns Subscription token (discard to unregister)
	///
	[[nodiscard]] Tk subscribe(Callback const& callback);
	///
	/// \brief Invoke registered callbacks; returns live count
	///
	void operator()(Args... args) const;
	///
	/// \brief Check if any registered callbacks exist
	/// \returns `true` if any previously distributed Token is still alive
	///
	[[nodiscard]] bool alive() const noexcept;
	///
	/// \brief Clear all registered callbacks
	///
	void clear() noexcept;

  private:
	Storage m_callbacks;
};

template <typename... Args>
typename Delegate<Args...>::Tk Delegate<Args...>::subscribe(Callback const& callback) {
	return m_callbacks.push(callback);
}

template <typename... Args>
void Delegate<Args...>::operator()(Args... args) const {
	for (auto const& callback : m_callbacks) { callback(args...); };
}

template <typename... Args>
bool Delegate<Args...>::alive() const noexcept {
	return !m_callbacks.empty();
}

template <typename... Args>
void Delegate<Args...>::clear() noexcept {
	m_callbacks.clear();
	return;
}
} // namespace le
