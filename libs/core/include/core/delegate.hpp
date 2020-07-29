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
	using Token = UniqueToken<s32>;
	using Callback = std::function<void(Args...)>;

private:
	Tokeniser<Callback, s32> m_tokeniser;

public:
	///
	/// \brief Register callback and obtain token
	/// \returns Subscription token (discard to unregister)
	///
	[[nodiscard]] Token subscribe(Callback callback);
	///
	/// \brief Clean up dead callbacks and invoke the rest
	/// \returns Count of registered callbacks
	///
	std::size_t operator()(Args... args);
	///
	/// \brief Skip dead callbacks and invoke the rest; returns live count
	/// \returns Count of registered callbacks
	///
	std::size_t operator()(Args... args) const;
	///
	/// \brief Check if any registered callbacks exist
	/// \returns `true` if any previously distributed Token is still alive
	///
	/// Calls `cleanup()`
	[[nodiscard]] bool isAlive();
	///
	/// \brief Clear all registered callbacks
	///
	void clear();
	///
	/// \brief Remove unregistered callbacks
	///
	void sweep();
};

template <typename... Args>
typename Delegate<Args...>::Token Delegate<Args...>::subscribe(Callback callback)
{
	return m_tokeniser.addUnique(std::move(callback));
}

template <typename... Args>
std::size_t Delegate<Args...>::operator()(Args... args)
{
	sweep();
	for (auto const& [callback, _] : m_tokeniser.entries)
	{
		callback(std::forward<Args>(args)...);
	}
	return m_tokeniser.size();
}

template <typename... Args>
std::size_t Delegate<Args...>::operator()(Args... args) const
{
	std::size_t ret = 0;
	for (auto const& [callback, token] : m_tokeniser.entries)
	{
		if (token.lock())
		{
			callback(args...);
			++ret;
		}
	}
	return ret;
}

template <typename... Args>
bool Delegate<Args...>::isAlive()
{
	sweep();
	return !m_tokeniser.empty();
}

template <typename... Args>
void Delegate<Args...>::clear()
{
	m_tokeniser.clear();
	return;
}

template <typename... Args>
void Delegate<Args...>::sweep()
{
	m_tokeniser.sweep();
	return;
}
} // namespace le
