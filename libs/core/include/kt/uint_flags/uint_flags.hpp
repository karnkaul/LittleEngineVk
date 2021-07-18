// KT header-only library
// Requirements: C++17

#pragma once
#include <cstdint>
#include <type_traits>

namespace kt {
///
/// \brief Wrapper around an integral type used as bit flags
///
template <typename Ty = std::uint32_t>
struct uint_flags {
	static_assert(std::is_integral_v<Ty>, "Ty must be integral");

	using type = Ty;

	///
	/// \brief Storage
	///
	type bits;

	///
	/// \brief Build an instance by setting t
	///
	template <typename... T>
	static constexpr uint_flags make(T... t) noexcept;

	///
	/// \brief Conversion operator
	///
	constexpr explicit operator Ty() const noexcept { return bits; }

	///
	/// \brief Add all inputs to this instance
	///
	template <typename... T>
	constexpr uint_flags& set(T... t) noexcept;
	///
	/// \brief Remove all inputs fron this instance
	///
	template <typename... T>
	constexpr uint_flags& reset(T... t) noexcept;

	///
	/// \brief Test if all bits in t are set
	///
	template <typename T>
	constexpr bool operator[](T t) const noexcept;
	///
	/// \brief Test if any bits are set
	///
	constexpr bool any() const noexcept { return bits != Ty{}; }
	///
	/// \brief Test if any bits in t are set
	///
	template <typename T>
	constexpr bool any(T t) const noexcept;
	///
	/// \brief Test if all bits in t are set
	///
	template <typename T>
	constexpr bool all(T t) const noexcept;
	///
	/// \brief Add set bits and remove unset bits
	///
	template <typename T, typename U = int>
	constexpr uint_flags& update(T set, U reset = U{}) noexcept;
};

// impl

template <typename Ty>
template <typename... T>
constexpr uint_flags<Ty> uint_flags<Ty>::make(T... t) noexcept {
	uint_flags ret{};
	ret.set(t...);
	return ret;
}
template <typename Ty>
template <typename... T>
constexpr uint_flags<Ty>& uint_flags<Ty>::set(T... t) noexcept {
	(update(t), ...);
	return *this;
}
template <typename Ty>
template <typename T>
constexpr bool uint_flags<Ty>::operator[](T t) const noexcept {
	return all(t);
}
template <typename Ty>
template <typename T>
constexpr bool uint_flags<Ty>::any(T t) const noexcept {
	return (bits & static_cast<Ty>(t)) != 0;
}
template <typename Ty>
template <typename T>
constexpr bool uint_flags<Ty>::all(T t) const noexcept {
	return (bits & static_cast<Ty>(t)) == static_cast<Ty>(t);
}
template <typename Ty>
template <typename T, typename U>
constexpr uint_flags<Ty>& uint_flags<Ty>::update(T set, U unset) noexcept {
	bits |= static_cast<Ty>(set);
	bits &= ~static_cast<Ty>(unset);
	return *this;
}
} // namespace kt
