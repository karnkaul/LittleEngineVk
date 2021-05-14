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
	/// \brief Build an instance by adding all inputs
	///
	template <typename... T>
	static constexpr uint_flags combine(T... t) noexcept;
	///
	/// \brief Build an instance by filling count lowest significant bits
	///
	static constexpr uint_flags fill(std::uint8_t count) noexcept;
	///
	/// \brief Add all inputs to this instance
	///
	template <typename T, typename... Ts>
	constexpr uint_flags& add(T t, Ts... ts) noexcept;

	///
	/// \brief Test if all bits in t are set
	///
	template <typename T>
	constexpr bool operator[](T t) const noexcept;
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
	constexpr uint_flags& update(T set, U unset = 0) noexcept;

	///
	/// \brief Test if any bits in rhs are set
	///
	constexpr bool any(uint_flags rhs) const noexcept;
	///
	/// \brief Test if all bits in rhs are set
	///
	constexpr bool all(uint_flags rhs) const noexcept;
	///
	/// \brief Add set flags and remove unset bits
	///
	template <typename T = int>
	constexpr uint_flags& update(uint_flags set, T unset = 0) noexcept;

	///
	/// \brief Compare two uint_flags
	///
	friend constexpr bool operator==(uint_flags a, uint_flags b) noexcept { return a.bits == b.bits; }
	///
	/// \brief Compare two uint_flags
	///
	friend constexpr bool operator!=(uint_flags a, uint_flags b) noexcept { return !(a == b); }
};

// impl

template <typename Ty>
template <typename... T>
constexpr uint_flags<Ty> uint_flags<Ty>::combine(T... t) noexcept {
	uint_flags ret{};
	ret.add(t...);
	return ret;
}
template <typename Ty>
constexpr uint_flags<Ty> uint_flags<Ty>::fill(std::uint8_t count) noexcept {
	uint_flags ret{};
	for (std::uint8_t i = 0; i < count; ++i) {
		ret.update(1 << i);
	}
	return ret;
}
template <typename Ty>
template <typename T, typename... Ts>
constexpr uint_flags<Ty>& uint_flags<Ty>::add(T t, Ts... ts) noexcept {
	update(t);
	if constexpr (sizeof...(Ts) > 0) {
		add(ts...);
	}
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
	return (bits & static_cast<type>(t)) != 0;
}
template <typename Ty>
template <typename T>
constexpr bool uint_flags<Ty>::all(T t) const noexcept {
	return (bits & static_cast<type>(t)) == static_cast<type>(t);
}
template <typename Ty>
template <typename T, typename U>
constexpr uint_flags<Ty>& uint_flags<Ty>::update(T set, U unset) noexcept {
	bits |= static_cast<type>(set);
	bits &= ~static_cast<type>(unset);
	return *this;
}
template <typename Ty>
constexpr bool uint_flags<Ty>::any(uint_flags rhs) const noexcept {
	return any(rhs.bits);
}
template <typename Ty>
constexpr bool uint_flags<Ty>::all(uint_flags rhs) const noexcept {
	return all(rhs.bits);
}
template <typename Ty>
template <typename T>
constexpr uint_flags<Ty>& uint_flags<Ty>::update(uint_flags set, T unset) noexcept {
	return update(set.bits, unset);
}
} // namespace kt
