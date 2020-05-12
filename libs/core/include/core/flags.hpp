#pragma once
#include <algorithm>
#include <bitset>
#include <initializer_list>
#include <string>
#include <type_traits>
#include "core/assert.hpp"
#include "core/std_types.hpp"

namespace le
{
template <typename Enum, size_t N = (size_t)Enum::eCOUNT_>
struct TFlags
{
	static_assert(std::is_enum_v<Enum>, "Enum must be an enum!");

	using Type = Enum;
	static constexpr size_t size = N;

	std::bitset<N> bits;

	constexpr TFlags() noexcept = default;

	constexpr /*implicit*/ TFlags(Enum flag) noexcept
	{
		set(flag);
	}

	constexpr TFlags(std::initializer_list<Enum> flags) noexcept
	{
		set(flags);
	}

	bool isSet(Enum flag) const
	{
		ASSERT((size_t)flag < N, "Invalid flag!");
		return bits.test((size_t)flag);
	}

	void set(Enum flag)
	{
		ASSERT((size_t)flag < N, "Invalid flag!");
		bits.set((size_t)flag);
	}

	void reset(Enum flag)
	{
		ASSERT((size_t)flag < N, "Invalid flag!");
		bits.reset((size_t)flag);
	}

	void flip(Enum flag)
	{
		ASSERT((size_t)flag < N, "Invalid flag");
		bits.flip((size_t)flag);
	}

	typename std::bitset<N>::reference operator[](Enum flag)
	{
		ASSERT((size_t)flag < N, "Invalid flag!");
		return bits[(size_t)flag];
	}

	bool allSet(std::initializer_list<Enum> flags) const
	{
		return std::all_of(flags.begin(), flags.end(), [&](Enum flag) { return isSet(flag); });
	}

	void set(std::initializer_list<Enum> flags)
	{
		for (auto flag : flags)
		{
			set(flag);
		}
	}

	void reset(std::initializer_list<Enum> flags)
	{
		for (auto flag : flags)
		{
			reset(flag);
		}
	}

	void set()
	{
		bits.set();
	}

	void reset()
	{
		bits.reset();
	}

	TFlags<Enum, N>& operator|=(TFlags<Enum, N> flags)
	{
		bits |= flags.bits;
		return *this;
	}

	TFlags<Enum, N>& operator&=(TFlags<Enum, N> flags)
	{
		bits &= flags.bits;
		return *this;
	}

	TFlags<Enum, N> operator|(TFlags<Enum, N> flags) const
	{
		auto ret = *this;
		ret |= flags;
		return ret;
	}

	TFlags<Enum, N> operator&(TFlags<Enum, N> flags) const
	{
		auto ret = *this;
		ret &= flags;
		return ret;
	}
};

template <typename Enum, size_t N = (size_t)Enum::eCOUNT_>
TFlags<Enum, N> operator|(Enum flag1, Enum flag2)
{
	return TFlags<Enum, N>(flag1) | TFlags<Enum, N>(flag2);
}

template <typename Enum, size_t N = (size_t)Enum::eCOUNT_>
TFlags<Enum, N> operator&(Enum flag1, Enum flag2)
{
	return TFlags<Enum, N>(flag1) & TFlags<Enum, N>(flag2);
}

template <typename Enum, size_t N = (size_t)Enum::eCOUNT_>
bool operator==(TFlags<Enum, N> lhs, TFlags<Enum, N> rhs)
{
	return lhs.bits == rhs.bits;
}

template <typename Enum, size_t N = (size_t)Enum::eCOUNT_>
bool operator!=(TFlags<Enum, N> lhs, TFlags<Enum, N> rhs)
{
	return !(lhs == rhs);
}
} // namespace le
