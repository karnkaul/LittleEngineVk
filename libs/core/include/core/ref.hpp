#pragma once

namespace le {
///
/// \brief Ultra-light reference wrapper
///
template <typename T>
struct Ref {
	using type = T;

	T* pPtr;

	constexpr Ref(T& t) noexcept;

	constexpr operator T&() const noexcept;
	constexpr T& get() const noexcept;
};

template <typename T>
constexpr bool operator==(Ref<T> lhs, Ref<T> rhs) noexcept;
template <typename T>
constexpr bool operator!=(Ref<T> lhs, Ref<T> rhs) noexcept;

template <typename T>
constexpr Ref<T>::Ref(T& t) noexcept : pPtr(&t) {
}

template <typename T>
constexpr Ref<T>::operator T&() const noexcept {
	return *pPtr;
}

template <typename T>
constexpr T& Ref<T>::get() const noexcept {
	return *pPtr;
}

template <typename T>
constexpr bool operator==(Ref<T> lhs, Ref<T> rhs) noexcept {
	return &lhs.get() == &rhs.get();
}

template <typename T>
constexpr bool operator!=(Ref<T> lhs, Ref<T> rhs) noexcept {
	return &lhs.get() != &rhs.get();
}
} // namespace le
