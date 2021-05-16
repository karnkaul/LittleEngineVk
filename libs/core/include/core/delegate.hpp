#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <core/tagged_store.hpp>

namespace le {
///
/// \brief Wrapper for invocation of multiple registered callbacks
///
template <typename... Args>
class Delegate {
  public:
	using Callback = std::function<void(Args...)>;
	using Tk = Tag<>;

  public:
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
	TaggedStore<Callback> m_tokens;
};

template <typename... Args>
typename Delegate<Args...>::Tk Delegate<Args...>::subscribe(Callback const& callback) {
	return m_tokens.emplace_back(callback);
}

template <typename... Args>
void Delegate<Args...>::operator()(Args... args) const {
	for (auto const& [_, callback] : m_tokens) { callback(args...); };
}

template <typename... Args>
bool Delegate<Args...>::alive() const noexcept {
	return !m_tokens.empty();
}

template <typename... Args>
void Delegate<Args...>::clear() noexcept {
	m_tokens.clear();
	return;
}
} // namespace le
