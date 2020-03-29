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
	using Result = TResult<Value*>;
	using CResult = TResult<Value const*>;

private:
	using Lock = std::scoped_lock<std::mutex>;

public:
	MapContainer m_map;

private:
	mutable std::mutex m_mutex;

public:
	void insert(Key const& id, Value&& value);
	[[nodiscard]] CResult get(Key const& id) const;
	[[nodiscard]] Result get(Key const& id);
	[[nodiscard]] bool isLoaded(Key const& id) const;
	bool unload(Key const& id);
	void unloadAll();
	u32 count() const;
};

template <typename MapContainer>
void TMapStore<MapContainer>::insert(Key const& id, Value&& value)
{
	Lock lock(m_mutex);
	m_map.emplace(id, std::forward<Value&&>(value));
}

template <typename MapContainer>
typename TMapStore<MapContainer>::CResult TMapStore<MapContainer>::get(Key const& id) const
{
	Lock lock(m_mutex);
	auto search = m_map.find(id);
	if (search != m_map.end())
	{
		return &search->second;
	}
	return {};
}

template <typename MapContainer>
typename TMapStore<MapContainer>::Result TMapStore<MapContainer>::get(Key const& id)
{
	Lock lock(m_mutex);
	auto search = m_map.find(id);
	if (search != m_map.end())
	{
		return &search->second;
	}
	return {};
}

template <typename MapContainer>
bool TMapStore<MapContainer>::isLoaded(Key const& id) const
{
	Lock lock(m_mutex);
	return m_map.find(id) != m_map.end();
}

template <typename MapContainer>
bool TMapStore<MapContainer>::unload(Key const& id)
{
	Lock lock(m_mutex);
	auto search = m_map.find(id);
	if (search != m_map.end())
	{
		m_map.erase(search);
		return true;
	}
	return false;
}

template <typename MapContainer>
void TMapStore<MapContainer>::unloadAll()
{
	Lock lock(m_mutex);
	m_map.clear();
	return;
}

template <typename MapContainer>
u32 TMapStore<MapContainer>::count() const
{
	Lock lock(m_mutex);
	return (u32)m_map.size();
}
} // namespace le
