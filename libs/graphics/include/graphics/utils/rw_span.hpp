#pragma once
#include <array>
#include <vector>
#include <core/ensure.hpp>

namespace le::graphics {
template <typename T>
struct RWSpan {
	using value_type = T;
	using iterator = T*;
	using const_iterator = T const*;

	T* pData;
	std::size_t extent;

	constexpr RWSpan() noexcept;
	constexpr explicit RWSpan(T* pData, std::size_t extent) noexcept;
	constexpr RWSpan(T& data) noexcept;
	template <std::size_t N>
	constexpr RWSpan(std::array<T, N>& arr) noexcept;
	template <std::size_t N>
	constexpr RWSpan(T (&arr)[N]) noexcept;
	constexpr RWSpan(std::vector<T>& vec) noexcept;

	constexpr std::size_t size() const noexcept;
	constexpr T const* data() const noexcept;
	constexpr T* data() noexcept;
	constexpr bool empty() const noexcept;
	constexpr const_iterator begin() const noexcept;
	constexpr const_iterator end() const noexcept;
	constexpr iterator begin() noexcept;
	constexpr iterator end() noexcept;

	T const& at(std::size_t idx) const;
	T const& operator[](std::size_t idx) const;
	T& at(std::size_t idx);
	T& operator[](std::size_t idx);
};

template <typename T>
constexpr RWSpan<T>::RWSpan() noexcept : pData(nullptr), extent(0) {
}

template <typename T>
constexpr RWSpan<T>::RWSpan(T* pData, std::size_t extent) noexcept : pData(pData), extent(extent) {
}

template <typename T>
constexpr RWSpan<T>::RWSpan(T& data) noexcept : pData(&data), extent(1) {
}

template <typename T>
template <std::size_t N>
constexpr RWSpan<T>::RWSpan(std::array<T, N>& arr) noexcept : pData(N == 0 ? nullptr : &arr.front()), extent(N) {
}

template <typename T>
template <std::size_t N>
constexpr RWSpan<T>::RWSpan(T (&arr)[N]) noexcept : pData(N == 0 ? nullptr : &arr[0]), extent(N) {
}

template <typename T>
constexpr RWSpan<T>::RWSpan(std::vector<T>& vec) noexcept : pData(vec.empty() ? nullptr : &vec.front()), extent(vec.size()) {
}

template <typename T>
constexpr std::size_t RWSpan<T>::size() const noexcept {
	return extent;
}

template <typename T>
constexpr bool RWSpan<T>::empty() const noexcept {
	return extent == 0;
}

template <typename T>
constexpr T const* RWSpan<T>::data() const noexcept {
	return pData;
}

template <typename T>
constexpr T* RWSpan<T>::data() noexcept {
	return pData;
}

template <typename T>
constexpr typename RWSpan<T>::const_iterator RWSpan<T>::begin() const noexcept {
	return pData;
}

template <typename T>
constexpr typename RWSpan<T>::const_iterator RWSpan<T>::end() const noexcept {
	return pData + extent;
}

template <typename T>
constexpr typename RWSpan<T>::iterator RWSpan<T>::begin() noexcept {
	return pData;
}

template <typename T>
constexpr typename RWSpan<T>::iterator RWSpan<T>::end() noexcept {
	return pData + extent;
}

template <typename T>
T const& RWSpan<T>::at(std::size_t idx) const {
	ENSURE(idx < extent, "OOB access!");
	return *(pData + idx);
}

template <typename T>
T const& RWSpan<T>::operator[](std::size_t idx) const {
	ENSURE(idx < extent, "OOB access!");
	return *(pData + idx);
}

template <typename T>
T& RWSpan<T>::at(std::size_t idx) {
	ENSURE(idx < extent, "OOB access!");
	return *(pData + idx);
}

template <typename T>
T& RWSpan<T>::operator[](std::size_t idx) {
	ENSURE(idx < extent, "OOB access!");
	return *(pData + idx);
}
} // namespace le::graphics
