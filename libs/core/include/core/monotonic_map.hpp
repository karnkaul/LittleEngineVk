#pragma once
#include <unordered_map>
#include <core/std_types.hpp>

namespace le {
struct MonoMap_Storage {
	using id_type = u64;
	static constexpr id_type zero = 0;

	template <typename T>
	using type = std::unordered_map<id_type, T>;

	static constexpr id_type increment(id_type id) noexcept;
};

template <typename T, typename S = MonoMap_Storage>
class MonotonicMap {
  public:
	using value_type = T;
	using id_type = typename S::id_type;
	using map_type = typename S::template type<value_type>;
	struct iterator;
	struct const_iterator;

	template <typename... U>
	id_type push(U&&... value);
	bool pop(id_type id);
	void clear();
	std::size_t size() const noexcept;
	id_type nextID() const noexcept;

	const_iterator begin() const;
	const_iterator end() const;
	iterator begin();
	iterator end();

	bool contains(id_type id) const;
	const_iterator find(id_type id) const;
	iterator find(id_type id);

  protected:
	map_type m_map;
	id_type m_nextID = S::zero;
};

template <typename T, typename S>
struct MonotonicMap<T, S>::const_iterator {
	using type = T;
	using iterator_type = typename MonotonicMap<T, S>::map_type::const_iterator;
	using pointer = T const*;
	using reference = T const&;
	iterator_type iter;

	explicit const_iterator(iterator_type iter) : iter(iter) {
	}
	reference operator*() const {
		return iter->second;
	}
	pointer operator->() const {
		return &iter->second;
	}
	const_iterator& operator++() {
		++iter;
		return *this;
	}
	friend bool operator==(const_iterator lhs, const_iterator rhs) noexcept {
		return lhs.iter == rhs.iter;
	}
	friend bool operator!=(const_iterator lhs, const_iterator rhs) noexcept {
		return !(lhs == rhs);
	}
};
template <typename T, typename S>
struct MonotonicMap<T, S>::iterator {
	using type = T;
	using iterator_type = typename MonotonicMap<T, S>::map_type::iterator;
	using pointer = T*;
	using reference = T&;
	iterator_type iter;

	explicit iterator(iterator_type iter) : iter(iter) {
	}
	reference operator*() {
		return iter->second;
	}
	pointer operator->() {
		return &iter->second;
	}
	iterator& operator++() {
		++iter;
		return *this;
	}
	friend bool operator==(iterator lhs, iterator rhs) noexcept {
		return lhs.iter == rhs.iter;
	}
	friend bool operator!=(iterator lhs, iterator rhs) noexcept {
		return !(lhs == rhs);
	}
};

inline constexpr MonoMap_Storage::id_type MonoMap_Storage::increment(id_type id) noexcept {
	return id + 1;
}

template <typename T, typename S>
template <typename... U>
typename MonotonicMap<T, S>::id_type MonotonicMap<T, S>::push(U&&... value) {
	m_nextID = nextID();
	m_map.emplace(m_nextID, std::forward<U>(value)...);
	return m_nextID;
}
template <typename T, typename S>
bool MonotonicMap<T, S>::pop(id_type id) {
	if (auto search = m_map.find(id); search != m_map.end()) {
		m_map.erase(search);
		return true;
	}
	return false;
}
template <typename T, typename S>
void MonotonicMap<T, S>::clear() {
	m_map.clear();
}
template <typename T, typename S>
std::size_t MonotonicMap<T, S>::size() const noexcept {
	return m_map.size();
}
template <typename T, typename S>
typename MonotonicMap<T, S>::id_type MonotonicMap<T, S>::nextID() const noexcept {
	return S::increment(m_nextID);
}
template <typename T, typename S>
typename MonotonicMap<T, S>::const_iterator MonotonicMap<T, S>::begin() const {
	return const_iterator(m_map.begin());
}
template <typename T, typename S>
typename MonotonicMap<T, S>::const_iterator MonotonicMap<T, S>::end() const {
	return const_iterator(m_map.end());
}
template <typename T, typename S>
typename MonotonicMap<T, S>::iterator MonotonicMap<T, S>::begin() {
	return iterator(m_map.begin());
}
template <typename T, typename S>
typename MonotonicMap<T, S>::iterator MonotonicMap<T, S>::end() {
	return iterator(m_map.end());
}
template <typename T, typename S>
bool MonotonicMap<T, S>::contains(id_type id) const {
	if (auto search = m_map.find(id); search != m_map.end()) {
		return true;
	}
	return false;
}
template <typename T, typename S>
typename MonotonicMap<T, S>::iterator MonotonicMap<T, S>::find(id_type id) {
	if (auto search = m_map.find(id); search != m_map.end()) {
		return iterator(search);
	}
	return iterator(m_map.end());
}
template <typename T, typename S>
typename MonotonicMap<T, S>::const_iterator MonotonicMap<T, S>::find(id_type id) const {
	if (auto search = m_map.find(id); search != m_map.end()) {
		return iterator(search);
	}
	return iterator(m_map.end());
}
} // namespace le
