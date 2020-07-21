#pragma once
#include <algorithm>
#include <cstdint>
#include <memory>
#include <functional>

namespace le
{
///
/// \brief Wrapper for invocation of multiple registered callbacks
///
template <typename... Args>
class Delegate
{
public:
	using Token = std::shared_ptr<int32_t>;
	using Callback = std::function<void(Args...)>;

private:
	using WToken = std::weak_ptr<int32_t>;

private:
	struct Wrapper
	{
		Callback callback;
		WToken wToken;
	};

private:
	std::vector<Wrapper> m_callbacks;

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
	uint32_t operator()(Args... t);
	///
	/// \brief Skip dead callbacks and invoke the rest; returns live count
	/// \returns Count of registered callbacks
	///
	uint32_t operator()(Args... t) const;
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
	void cleanup();
};

template <typename... Args>
typename Delegate<Args...>::Token Delegate<Args...>::subscribe(Callback callback)
{
	Token token = std::make_shared<int32_t>(int32_t(m_callbacks.size()));
	m_callbacks.push_back({std::move(callback), token});
	return token;
}

template <typename... Args>
uint32_t Delegate<Args...>::operator()(Args... t)
{
	cleanup();
	for (auto const& c : m_callbacks)
	{
		c.callback(t...);
	}
	return uint32_t(m_callbacks.size());
}

template <typename... Args>
uint32_t Delegate<Args...>::operator()(Args... t) const
{
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

template <typename... Args>
bool Delegate<Args...>::isAlive()
{
	cleanup();
	return !m_callbacks.empty();
}

template <typename... Args>
void Delegate<Args...>::clear()
{
	m_callbacks.clear();
	return;
}

template <typename... Args>
void Delegate<Args...>::cleanup()
{
	auto iter = std::remove_if(m_callbacks.begin(), m_callbacks.end(), [](Wrapper& wrapper) -> bool { return wrapper.wToken.expired(); });
	m_callbacks.erase(iter, m_callbacks.end());
	return;
}
} // namespace le
