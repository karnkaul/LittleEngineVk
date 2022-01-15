#pragma once
#include <levk/core/utils/error.hpp>
#include <vector>

namespace le {
template <typename T>
struct TRotator {
	using type = T;

	template <typename... U>
	void emplace(U&&... u);
	void push(T&& t) { emplace(std::move(t)); }
	void push(T const& t) { emplace(t); }

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
void TRotator<T>::emplace(U&&... u) {
	ts.emplace_back(std::forward<U>(u)...);
}
template <typename T>
typename TRotator<T>::type& TRotator<T>::get() {
	ENSURE(!ts.empty(), "Empty buffer");
	return ts[index];
}
template <typename T>
typename TRotator<T>::type const& TRotator<T>::get() const {
	ENSURE(!ts.empty(), "Empty buffer");
	return ts[index];
}
template <typename T>
void TRotator<T>::next() noexcept {
	if (!ts.empty()) {
		index = (index + 1) % size();
		++total;
	}
}
template <typename T>
std::size_t TRotator<T>::size() const noexcept {
	return ts.size();
}
template <typename T>
bool TRotator<T>::empty() const noexcept {
	return ts.empty();
}
} // namespace le
