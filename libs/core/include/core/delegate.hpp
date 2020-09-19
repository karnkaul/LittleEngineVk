#pragma once
#include <cstdint>
#include <functional>
#include <core/tokeniser.hpp>

namespace le
{
///
/// \brief Wrapper for invocation of multiple registered callbacks
///
template <typename... Args>
class Delegate
{
public:
	using Callback = std::function<void(Args...)>;
	using Tk = Token;

public:
	///
	/// \brief Register callback and obtain token
	/// \returns Subscription token (discard to unregister)
	///
	[[nodiscard]] Tk subscribe(Callback callback);
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
	TTokenGen<Callback, std::vector> m_tokeniser;
};

template <typename... Args>
typename Delegate<Args...>::Tk Delegate<Args...>::subscribe(Callback callback)
{
	return m_tokeniser.push(std::move(callback));
}

template <typename... Args>
void Delegate<Args...>::operator()(Args... args) const
{
	m_tokeniser.forEach([&args...](auto& callback) { callback(args...); });
}

template <typename... Args>
bool Delegate<Args...>::alive() const noexcept
{
	return !m_tokeniser.empty();
}

template <typename... Args>
void Delegate<Args...>::clear() noexcept
{
	m_tokeniser.clear();
	return;
}
} // namespace le
