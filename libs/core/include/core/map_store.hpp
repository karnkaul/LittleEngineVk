#pragma once
#include <map>
#include <mutex>
#include <unordered_map>
#include "std_types.hpp"

namespace le
{
template <typename MapContainer>
class TMapStore final
{
public:
	using Key = typename MapContainer::key_type;
	using Value = typename MapContainer::mapped_type;

public:
	MapContainer m_map;

private:
	mutable std::mutex m_mutex;

public:
	void insert(Key const& id, Value&& value);
	[[nodiscard]] TResult<Value const*> get(Key const& id) const;
	[[nodiscard]] TResult<Value*> get(Key const& id);
	[[nodiscard]] bool isLoaded(Key const& id) const;
	bool unload(Key const& id);
	void unloadAll();
	u32 count() const;
};

template <typename M>
void TMapStore<M>::insert(Key const& id, Value&& value)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	m_map.emplace(id, std::forward<Value&&>(value));
}

template <typename M>
TResult<typename TMapStore<M>::Value const*> TMapStore<M>::get(Key const& id) const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	auto search = m_map.find(id);
	if (search != m_map.end())
	{
		return &search->second;
	}
	return {};
}

template <typename M>
TResult<typename TMapStore<M>::Value*> TMapStore<M>::get(Key const& id)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	auto search = m_map.find(id);
	if (search != m_map.end())
	{
		return &search->second;
	}
	return {};
}

template <typename M>
bool TMapStore<M>::isLoaded(Key const& id) const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	return m_map.find(id) != m_map.end();
}

template <typename M>
bool TMapStore<M>::unload(Key const& id)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	auto search = m_map.find(id);
	if (search != m_map.end())
	{
		m_map.erase(search);
		return true;
	}
	return false;
}

template <typename M>
void TMapStore<M>::unloadAll()
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	m_map.clear();
	return;
}

template <typename M>
u32 TMapStore<M>::count() const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	return (u32)m_map.size();
}
} // namespace le
