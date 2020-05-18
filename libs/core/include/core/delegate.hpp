#pragma once
#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <functional>

namespace le
{
template <typename... Args>
class Delegate
{
public:
	using Token = std::shared_ptr<int32_t>;
	using Callback = std::function<void(Args... t)>;

private:
	using WToken = std::weak_ptr<int32_t>;
	using Lock = std::scoped_lock<std::mutex>;

private:
	struct Wrapper
	{
		Callback callback;
		WToken wToken;
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

template <typename... Args>
typename Delegate<Args...>::Token Delegate<Args...>::subscribe(Callback callback)
{
	Lock lock(m_mutex);
	Token token = std::make_shared<int32_t>(int32_t(m_callbacks.size()));
	m_callbacks.push_back({std::move(callback), token});
	return token;
}

template <typename... Args>
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

template <typename... Args>
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

template <typename... Args>
bool Delegate<Args...>::isAlive()
{
	cleanup();
	Lock lock(m_mutex);
	return !m_callbacks.empty();
}

template <typename... Args>
void Delegate<Args...>::clear()
{
	Lock lock(m_mutex);
	m_callbacks.clear();
	return;
}

template <typename... Args>
void Delegate<Args...>::cleanup()
{
	Lock lock(m_mutex);
	auto iter = std::remove_if(m_callbacks.begin(), m_callbacks.end(), [](Wrapper& wrapper) -> bool { return wrapper.wToken.expired(); });
	m_callbacks.erase(iter, m_callbacks.end());
	return;
}
} // namespace le
