#pragma once
#include <array>
#include <vector>
#include <core/ensure.hpp>

namespace le {
///
/// \brief View-only class for an object / a contiguous range of objects
///
template <typename T>
struct Span {
	using value_type = T;
	using const_iterator = T const*;

	T const* pData;
	std::size_t extent;

	constexpr Span() noexcept;
	constexpr explicit Span(T const* pData, std::size_t extent) noexcept;
	constexpr Span(T const& data) noexcept;
	template <std::size_t N>
	constexpr Span(std::array<T, N> const& arr) noexcept;
	template <std::size_t N>
	constexpr Span(T const (&arr)[N]) noexcept;
	constexpr Span(std::vector<T> const& vec) noexcept;

	constexpr std::size_t size() const noexcept;
	constexpr bool empty() const noexcept;
	constexpr const_iterator begin() const noexcept;
	constexpr const_iterator end() const noexcept;

	T const& at(std::size_t idx) const;
	T const& operator[](std::size_t idx) const;
};

template <typename T>
constexpr Span<T>::Span() noexcept : pData(nullptr), extent(0) {
}

template <typename T>
constexpr Span<T>::Span(T const* pData, std::size_t extent) noexcept : pData(pData), extent(extent) {
}

template <typename T>
constexpr Span<T>::Span(T const& data) noexcept : pData(&data), extent(1) {
}

template <typename T>
template <std::size_t N>
constexpr Span<T>::Span(std::array<T, N> const& arr) noexcept : pData(N == 0 ? nullptr : &arr.front()), extent(N) {
}

template <typename T>
template <std::size_t N>
constexpr Span<T>::Span(T const (&arr)[N]) noexcept : pData(N == 0 ? nullptr : &arr[0]), extent(N) {
}

template <typename T>
constexpr Span<T>::Span(std::vector<T> const& vec) noexcept : pData(vec.empty() ? nullptr : &vec.front()), extent(vec.size()) {
}

template <typename T>
constexpr std::size_t Span<T>::size() const noexcept {
	return extent;
}

template <typename T>
constexpr bool Span<T>::empty() const noexcept {
	return extent == 0;
}

template <typename T>
constexpr typename Span<T>::const_iterator Span<T>::begin() const noexcept {
	return pData;
}

template <typename T>
constexpr typename Span<T>::const_iterator Span<T>::end() const noexcept {
	return pData + extent;
}

template <typename T>
T const& Span<T>::at(std::size_t idx) const {
	ENSURE(idx < extent, "OOB access!");
	return *(pData + idx);
}

template <typename T>
T const& Span<T>::operator[](std::size_t idx) const {
	ENSURE(idx < extent, "OOB access!");
	return *(pData + idx);
}
} // namespace le
