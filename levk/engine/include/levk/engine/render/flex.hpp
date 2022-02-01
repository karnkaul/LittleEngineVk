#pragma once

namespace le {
template <typename T>
struct TFlex {
	using type = T;

	T norm;
	T offset;

	constexpr TFlex(T const& norm = {}, T const& offset = {}) noexcept : norm(norm), offset(offset) {}

	constexpr TFlex<T>& operator+=(TFlex const& rhs) noexcept;
	constexpr TFlex<T>& operator-=(TFlex const& rhs) noexcept;
	constexpr TFlex<T>& operator*=(TFlex const& rhs) noexcept;
	constexpr TFlex<T>& operator/=(TFlex const& rhs) noexcept;
};

template <typename T>
constexpr TFlex<T> operator-(TFlex<T> const& flex) noexcept {
	return {-flex.norm, -flex.offset};
}

template <typename T>
constexpr TFlex<T> operator+(TFlex<T> const& lhs, TFlex<T> const& rhs) noexcept {
	return {lhs.norm + rhs.norm, lhs.offset + rhs.offset};
}

template <typename T>
constexpr TFlex<T> operator-(TFlex<T> const& lhs, TFlex<T> const& rhs) noexcept {
	return {lhs.norm - rhs.norm, lhs.offset - rhs.offset};
}

template <typename T>
constexpr TFlex<T> operator*(TFlex<T> const& lhs, TFlex<T> const& rhs) noexcept {
	return {lhs.norm * rhs.norm, lhs.offset * rhs.offset};
}

template <typename T>
constexpr TFlex<T> operator/(TFlex<T> const& lhs, TFlex<T> const& rhs) noexcept {
	return {lhs.norm / rhs.norm, lhs.offset / rhs.offset};
}

// impl

template <typename T>
constexpr TFlex<T>& TFlex<T>::operator+=(TFlex const& rhs) noexcept {
	norm += rhs.norm;
	offset += rhs.offset;
	return *this;
}
template <typename T>
constexpr TFlex<T>& TFlex<T>::operator-=(TFlex const& rhs) noexcept {
	norm -= rhs.norm;
	offset -= rhs.offset;
	return *this;
}
template <typename T>
constexpr TFlex<T>& TFlex<T>::operator*=(TFlex const& rhs) noexcept {
	norm *= rhs.norm;
	offset *= rhs.offset;
	return *this;
}
template <typename T>
constexpr TFlex<T>& TFlex<T>::operator/=(TFlex const& rhs) noexcept {
	norm /= rhs.norm;
	offset /= rhs.offset;
	return *this;
}
} // namespace le
