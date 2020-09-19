#pragma once
#include <algorithm>
#include <bitset>
#include <type_traits>
#include <core/assert.hpp>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Type-safe and index-safe wrapper for `std::bitset`
/// \param `Enum`: enum [class] type
/// \param `N`: number of bits (defaults to Enum::eCOUNT_)
///
template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
struct TFlags
{
	static_assert(std::is_enum_v<Enum>, "Enum must be an enum!");

	using type = Enum;
	static constexpr std::size_t size = N;

	std::bitset<N> bits;

	static constexpr TFlags<Enum, N> inverse() noexcept;

	constexpr TFlags() noexcept = default;
	///
	/// \brief Construct with a single flag set
	///
	constexpr /*implicit*/ TFlags(Enum flag) noexcept;

	///
	/// \brief Check if passed flags are set
	///
	constexpr bool test(TFlags<Enum, N> flags) const noexcept;
	///
	/// \brief Obtain count of set flags
	///
	constexpr std::size_t test() const noexcept;
	///
	/// \brief Set passed flags
	///
	constexpr TFlags<Enum, N>& set(TFlags<Enum, N> flags) noexcept;
	///
	/// \brief Reset passed flags
	///
	constexpr TFlags<Enum, N>& reset(TFlags<Enum, N> flags) noexcept;
	///
	/// \brief Flip passed flags
	///
	constexpr TFlags<Enum, N>& flip(TFlags<Enum, N> flags) noexcept;
	///
	/// \brief Set all flags
	///
	constexpr TFlags<Enum, N>& set() noexcept;
	///
	/// \brief Reset all flags
	///
	constexpr TFlags<Enum, N>& reset() noexcept;
	//
	/// \brief Flip all flags
	///
	constexpr TFlags<Enum, N>& flip() noexcept;
	///
	/// \brief Check if all passed flags are set
	///
	constexpr bool all(TFlags<Enum, N> flags = inverse()) const noexcept;
	///
	/// \brief Check if any passed flags are set
	///
	constexpr bool any(TFlags<Enum, N> flags = inverse()) const noexcept;
	///
	/// \brief Check if no passed flags are set
	///
	constexpr bool none(TFlags<Enum, N> flags = inverse()) const noexcept;

	///
	/// \brief Obtain a reference to the flag
	///
	constexpr typename std::bitset<N>::reference operator[](Enum flag) noexcept;
	///
	/// \brief Perform bitwise `OR` / add flags
	///
	constexpr TFlags<Enum, N>& operator|=(TFlags<Enum, N> flags) noexcept;
	///
	/// \brief Perform bitwise `AND` / multiply flags
	///
	constexpr TFlags<Enum, N>& operator&=(TFlags<Enum, N> flags) noexcept;
	///
	/// \brief Perform bitwise `XOR` / exclusively add flags (add mod 2)
	///
	constexpr TFlags<Enum, N>& operator^=(TFlags<Enum, N> flags) noexcept;
	///
	/// \brief Perform bitwise `OR` / add flags
	///
	constexpr TFlags<Enum, N> operator|(TFlags<Enum, N> flags) const noexcept;
	///
	/// \brief Perform bitwise `AND` / multiply flags
	///
	constexpr TFlags<Enum, N> operator&(TFlags<Enum, N> flags) const noexcept;
	///
	/// \brief Perform bitwise `XOR` / exclusively add flags (add mod 2)
	///
	constexpr TFlags<Enum, N> operator^(TFlags<Enum, N> flags) const noexcept;
};

///
/// \brief Add two flags
///
template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
constexpr TFlags<Enum, N> operator|(Enum flag1, Enum flag2) noexcept;
///
/// \brief Multiply two flags
///
template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
constexpr TFlags<Enum, N> operator&(Enum flag1, Enum flag2) noexcept;
///
/// \brief Exclusively add two flags
///
template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
constexpr TFlags<Enum, N> operator^(Enum flag1, Enum flag2) noexcept;
///
/// \brief Test if flags are equal
///
template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
constexpr bool operator==(TFlags<Enum, N> lhs, TFlags<Enum, N> rhs) noexcept;
///
/// \brief Test if flags are not equal
///
template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
constexpr bool operator!=(TFlags<Enum, N> lhs, TFlags<Enum, N> rhs) noexcept;

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N> TFlags<Enum, N>::inverse() noexcept
{
	return TFlags<Enum, N>().flip();
}

template <typename Enum, std::size_t N>
constexpr /*implicit*/ TFlags<Enum, N>::TFlags(Enum flag) noexcept
{
	bits.set((std::size_t)flag);
}

template <typename Enum, std::size_t N>
constexpr bool TFlags<Enum, N>::test(TFlags<Enum, N> flags) const noexcept
{
	return (bits & flags.bits) == flags.bits;
}

template <typename Enum, std::size_t N>
constexpr std::size_t TFlags<Enum, N>::test() const noexcept
{
	return bits.count();
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N>& TFlags<Enum, N>::set(TFlags<Enum, N> flags) noexcept
{
	bits |= flags.bits;
	return *this;
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N>& TFlags<Enum, N>::reset(TFlags<Enum, N> flags) noexcept
{
	bits &= flags.bits.flip();
	return *this;
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N>& TFlags<Enum, N>::flip(TFlags<Enum, N> flags) noexcept
{
	bits ^= flags.bits;
	return *this;
}

template <typename Enum, std::size_t N>
constexpr bool TFlags<Enum, N>::all(TFlags<Enum, N> flags) const noexcept
{
	return (flags.bits & bits) == bits;
}

template <typename Enum, std::size_t N>
constexpr bool TFlags<Enum, N>::any(TFlags<Enum, N> flags) const noexcept
{
	return (flags.bits & bits).any();
}

template <typename Enum, std::size_t N>
constexpr bool TFlags<Enum, N>::none(TFlags<Enum, N> flags) const noexcept
{
	return (flags.bits & bits).none();
}

template <typename Enum, std::size_t N>
constexpr typename std::bitset<N>::reference TFlags<Enum, N>::operator[](Enum flag) noexcept
{
	ASSERT((std::size_t)flag < N, "Invalid flag!");
	return bits[(std::size_t)flag];
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N>& TFlags<Enum, N>::operator|=(TFlags<Enum, N> flags) noexcept
{
	bits |= flags.bits;
	return *this;
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N>& TFlags<Enum, N>::operator&=(TFlags<Enum, N> flags) noexcept
{
	bits &= flags.bits;
	return *this;
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N>& TFlags<Enum, N>::operator^=(TFlags<Enum, N> flags) noexcept
{
	bits ^= flags.bits;
	return *this;
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N> TFlags<Enum, N>::operator|(TFlags<Enum, N> flags) const noexcept
{
	auto ret = *this;
	ret |= flags;
	return ret;
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N> TFlags<Enum, N>::operator&(TFlags<Enum, N> flags) const noexcept
{
	auto ret = *this;
	ret &= flags;
	return ret;
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N> TFlags<Enum, N>::operator^(TFlags<Enum, N> flags) const noexcept
{
	auto ret = *this;
	ret ^= flags;
	return ret;
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N> operator|(Enum flag1, Enum flag2) noexcept
{
	return TFlags<Enum, N>(flag1) | TFlags<Enum, N>(flag2);
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N> operator&(Enum flag1, Enum flag2) noexcept
{
	return TFlags<Enum, N>(flag1) & TFlags<Enum, N>(flag2);
}

template <typename Enum, std::size_t N>
constexpr TFlags<Enum, N> operator^(Enum flag1, Enum flag2) noexcept
{
	return TFlags<Enum, N>(flag1) ^ TFlags<Enum, N>(flag2);
}

template <typename Enum, std::size_t N>
constexpr bool operator==(TFlags<Enum, N> lhs, TFlags<Enum, N> rhs) noexcept
{
	return lhs.bits == rhs.bits;
}

template <typename Enum, std::size_t N>
constexpr bool operator!=(TFlags<Enum, N> lhs, TFlags<Enum, N> rhs) noexcept
{
	return !(lhs == rhs);
}
} // namespace le
