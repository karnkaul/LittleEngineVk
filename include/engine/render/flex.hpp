#pragma once

namespace le {
template <typename T>
struct TFlex {
	using type = T;

	type norm;
	type offset;

	constexpr TFlex(type const& norm = {}, type const& offset = {}) noexcept;

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
constexpr TFlex<T>::TFlex(type const& norm, type const& offset) noexcept : norm(norm), offset(offset) {}

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
