#pragma once
#include <vector>
#include <core/ensure.hpp>

namespace le {
template <typename T>
struct RingBuffer {
	using type = T;

	template <typename... U>
	void push(U&&... u);

	T& get();
	T const& get() const;
	void next() noexcept;
	std::size_t size() const noexcept;
	bool empty() const noexcept;

	std::vector<T> ts;
	std::size_t total = 0;
	std::size_t index = 0;
};

template <typename T>
template <typename... U>
void RingBuffer<T>::push(U&&... u) {
	ts.emplace_back(std::forward<U>(u)...);
}
template <typename T>
typename RingBuffer<T>::type& RingBuffer<T>::get() {
	ENSURE(!ts.empty(), "Empty buffer");
	return ts[index];
}
template <typename T>
typename RingBuffer<T>::type const& RingBuffer<T>::get() const {
	ENSURE(!ts.empty(), "Empty buffer");
	return ts[index];
}
template <typename T>
void RingBuffer<T>::next() noexcept {
	if (!ts.empty()) {
		index = (index + 1) % size();
		++total;
	}
}
template <typename T>
std::size_t RingBuffer<T>::size() const noexcept {
	return ts.size();
}
template <typename T>
bool RingBuffer<T>::empty() const noexcept {
	return ts.empty();
}
} // namespace le
