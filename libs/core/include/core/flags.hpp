#pragma once
#include <algorithm>
#include <bitset>
#include <initializer_list>
#include <string>
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

	using Type = Enum;
	static constexpr std::size_t size = N;

	std::bitset<N> bits;

	constexpr TFlags() noexcept = default;

	///
	/// \brief Construct with a single flag set
	///
	constexpr /*implicit*/ TFlags(Enum flag) noexcept
	{
		set(flag);
	}

	///
	/// \brief Construct with a number of flags set
	///
	constexpr TFlags(std::initializer_list<Enum> flags) noexcept
	{
		set(flags);
	}

	///
	/// \brief Check if a flag is set
	///
	bool isSet(Enum flag) const noexcept
	{
		ASSERT((std::size_t)flag < N, "Invalid flag!");
		return bits.test((std::size_t)flag);
	}

	///
	/// \brief Set a flag
	///
	void set(Enum flag) noexcept
	{
		ASSERT((std::size_t)flag < N, "Invalid flag!");
		bits.set((std::size_t)flag);
	}

	///
	/// \brief Reset a flag
	///
	void reset(Enum flag) noexcept
	{
		ASSERT((std::size_t)flag < N, "Invalid flag!");
		bits.reset((std::size_t)flag);
	}

	///
	/// \brief Set a number of flags
	///
	void set(std::initializer_list<Enum> flags) noexcept
	{
		for (auto flag : flags)
		{
			set(flag);
		}
	}

	///
	/// \brief Reset a number of flags
	///
	void reset(std::initializer_list<Enum> flags) noexcept
	{
		for (auto flag : flags)
		{
			reset(flag);
		}
	}

	///
	/// \brief Set all flags
	///
	void set() noexcept
	{
		bits.set();
	}

	///
	/// \brief Reset all flags
	///
	void reset() noexcept
	{
		bits.reset();
	}

	///
	/// \brief Flip a flag
	///
	void flip(Enum flag) noexcept
	{
		ASSERT((std::size_t)flag < N, "Invalid flag");
		bits.flip((std::size_t)flag);
	}

	//
	/// \brief Flip all flags
	///
	void flip() noexcept
	{
		bits.flip();
	}

	///
	/// \brief Obtain a reference to the flag
	///
	typename std::bitset<N>::reference operator[](Enum flag) noexcept
	{
		ASSERT((std::size_t)flag < N, "Invalid flag!");
		return bits[(std::size_t)flag];
	}

	///
	/// \brief Check if all flags are set
	///
	bool allSet(std::initializer_list<Enum> flags) const noexcept
	{
		return std::all_of(flags.begin(), flags.end(), [&](Enum flag) { return isSet(flag); });
	}

	///
	/// \brief Check if any flag is set
	///
	bool anySet(std::initializer_list<Enum> flags) const noexcept
	{
		return std::all_of(flags.begin(), flags.end(), [&](Enum flag) { return isSet(flag); });
	}

	///
	/// \brief Check if no flags are set
	///
	bool noneSet(std::initializer_list<Enum> flags) const noexcept
	{
		return std::all_of(flags.begin(), flags.end(), [&](Enum flag) { return isSet(flag); });
	}

	///
	/// \brief Perform bitwise `OR`
	///
	TFlags<Enum, N>& operator|=(TFlags<Enum, N> flags) noexcept
	{
		bits |= flags.bits;
		return *this;
	}

	///
	/// \brief Perform bitwise `AND`
	///
	TFlags<Enum, N>& operator&=(TFlags<Enum, N> flags) noexcept
	{
		bits &= flags.bits;
		return *this;
	}

	///
	/// \brief Perform bitwise `OR`
	///
	TFlags<Enum, N> operator|(TFlags<Enum, N> flags) const noexcept
	{
		auto ret = *this;
		ret |= flags;
		return ret;
	}

	///
	/// \brief Perform bitwise `AND`
	///
	TFlags<Enum, N> operator&(TFlags<Enum, N> flags) const noexcept
	{
		auto ret = *this;
		ret &= flags;
		return ret;
	}
};

template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
TFlags<Enum, N> operator|(Enum flag1, Enum flag2)
{
	return TFlags<Enum, N>(flag1) | TFlags<Enum, N>(flag2);
}

template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
TFlags<Enum, N> operator&(Enum flag1, Enum flag2)
{
	return TFlags<Enum, N>(flag1) & TFlags<Enum, N>(flag2);
}

template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
bool operator==(TFlags<Enum, N> lhs, TFlags<Enum, N> rhs)
{
	return lhs.bits == rhs.bits;
}

template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
bool operator!=(TFlags<Enum, N> lhs, TFlags<Enum, N> rhs)
{
	return !(lhs == rhs);
}
} // namespace le
