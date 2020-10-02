#pragma once
#include <type_traits>
#include <utility>

namespace le {
///
/// \brief Structured Binding of a payload and a `bool` (indicating the result of an operation)
///
template <typename T>
struct TResult {
	static_assert(std::is_default_constructible_v<T>, "T must be default constructible!");

	using type = T;

	T payload;
	bool bResult;

	constexpr TResult() noexcept;
	constexpr TResult(T&& payload, bool bResult = true) noexcept;
	constexpr TResult(T const& payload, bool bResult = true) noexcept;

	constexpr operator bool() const noexcept;
	constexpr T const& operator*() const noexcept;
	constexpr T& operator*() noexcept;
	constexpr T const* operator->() const noexcept;
	constexpr T* operator->() noexcept;
};

template <>
struct TResult<void> {
	using type = void;

	bool bResult;

	constexpr TResult(bool bResult = false) noexcept;

	constexpr operator bool() const noexcept;
};

template <typename T>
constexpr TResult<T>::TResult() noexcept : payload(T{}), bResult(false) {
}

template <typename T>
constexpr TResult<T>::TResult(T&& payload, bool bResult) noexcept : payload(std::move(payload)), bResult(bResult) {
}

template <typename T>
constexpr TResult<T>::TResult(T const& payload, bool bResult) noexcept : payload(payload), bResult(bResult) {
}

template <typename T>
constexpr TResult<T>::operator bool() const noexcept {
	return bResult;
}

template <typename T>
constexpr T const& TResult<T>::operator*() const noexcept {
	return payload;
}

template <typename T>
constexpr T& TResult<T>::operator*() noexcept {
	return payload;
}

template <typename T>
constexpr T const* TResult<T>::operator->() const noexcept {
	return &payload;
}

template <typename T>
constexpr T* TResult<T>::operator->() noexcept {
	return &payload;
}

inline constexpr TResult<void>::TResult(bool bResult) noexcept : bResult(bResult) {
}

inline constexpr TResult<void>::operator bool() const noexcept {
	return bResult;
}

} // namespace le
