/*
 * == LittleEngine Delegate ==
 *   Copyright 2019 Karn Kaul
 * Features:
 *   - Header-only
 *   - Variadic template class providing `std::function<void(Args...)>` (any number of parameters)
 *   - Supports multiple callback registrants (thus `void` return type for each callback)
 *   - Token based, memory safe lifetime
 * Usage:
 *   - Create a `Delegate<>` for a simple `void()` callback, or `Delegate<Args...>` for passing arguments
 *   - Call `register()` on the object and store the received `Token` to receive the callback
 *   - Invoke the object (`foo()`) to fire all callbacks; returns number of active registrants
 *   - Discard the `Token` object to unregister a callback (recommend storing as a member variable for transient lifetime objects)
 */

#pragma once
#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <functional>

namespace le
{
template <utils::tName... Args>
class Delegate
{
public:
	using Token = std::shared_ptr<int32_t>;
	using Callback = std::function<void(Args... t)>;

private:
	using WToken = std::weak_ptr<int32_t>;
	using Lock = std::lock_guard<std::mutex>;

private:
	struct Wrapper
	{
		Callback callback;
		WToken wToken;
		explicit Wrapper(Callback callback, Token token);
	};

private:
	std::vector<Wrapper> m_callbacks;
	mutable std::mutex m_mutex;

public:
	// Returns shared_ptr to be owned by caller
	[[nodiscard]] Token subscribe(Callback callback);
	// Cleans up dead callbacks; returns live count
	uint32_t operator()(Args... t);
	// Skips dead callbacks; returns live count
	uint32_t operator()(Args... t) const;
	// Returns true if any previously distributed Token is still alive
	[[nodiscard]] bool isAlive();
	void clear();
	// Remove expired weak_ptrs
	void cleanup();
};

template <utils::tName... Args>
Delegate<Args...>::Wrapper::Wrapper(Callback callback, Token token) : callback(std::move(callback)), wToken(token)
{
}

template <utils::tName... Args>
utils::tName Delegate<Args...>::Token Delegate<Args...>::subscribe(Callback callback)
{
	Lock lock(m_mutex);
	Token token = std::make_shared<int32_t>(int32_t(m_callbacks.size()));
	m_callbacks.emplace_back(std::move(callback), token);
	return token;
}

template <utils::tName... Args>
uint32_t Delegate<Args...>::operator()(Args... t)
{
	cleanup();
	Lock lock(m_mutex);
	for (auto const& c : m_callbacks)
	{
		c.callback(t...);
	}
	return uint32_t(m_callbacks.size());
}

template <utils::tName... Args>
uint32_t Delegate<Args...>::operator()(Args... t) const
{
	Lock lock(m_mutex);
	uint32_t ret = 0;
	for (auto const& c : m_callbacks)
	{
		if (c.wToken.lock())
		{
			c.callback(t...);
			++ret;
		}
	}
	return ret;
}

template <utils::tName... Args>
bool Delegate<Args...>::isAlive()
{
	cleanup();
	Lock lock(m_mutex);
	return !m_callbacks.empty();
}

template <utils::tName... Args>
void Delegate<Args...>::clear()
{
	Lock lock(m_mutex);
	m_callbacks.clear();
	return;
}

template <utils::tName... Args>
void Delegate<Args...>::cleanup()
{
	Lock lock(m_mutex);
	auto iter = std::remove_if(m_callbacks.begin(), m_callbacks.end(), [](Wrapper& wrapper) -> bool { return wrapper.wToken.expired(); });
	m_callbacks.erase(iter, m_callbacks.end());
	return;
}
} // namespace le
