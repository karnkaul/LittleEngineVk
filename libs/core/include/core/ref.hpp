#pragma once
#include <functional>

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
constexpr bool operator==(Ref<T> lhs, Ref<T> rhs) noexcept {
	return lhs.pPtr == rhs.pPtr;
}

template <typename T>
constexpr bool operator!=(Ref<T> lhs, Ref<T> rhs) noexcept {
	return lhs.pPtr != rhs.pPtr;
}
} // namespace le

namespace std {
template <typename T>
struct hash<le::Ref<T>> {
	size_t operator()(le::Ref<T> const& lhs) const {
		return std::hash<T const*>()(lhs.pPtr);
	}
};
} // namespace std
