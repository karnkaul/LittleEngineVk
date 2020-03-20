#pragma once
#include <bitset>
#include <string>
#include <type_traits>
#include "core/assert.hpp"
#include "core/std_types.hpp"

namespace le
{
template <typename Enum, size_t N = (size_t)Enum::eCOUNT_, typename = std::enable_if_t<std::is_enum_v<Enum>>>
struct TFlags
{
	using Type = Enum;
	static constexpr size_t size = N;

	std::bitset<N> bits;

	constexpr TFlags() noexcept = default;

	template <typename Flag1, typename... FlagN>
	constexpr TFlags(Flag1 flag1, FlagN... flagN) noexcept
	{
		set(flag1, flagN...);
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

	template <typename Flag1, typename... FlagN>
	bool isSet(Flag1 flag1, FlagN... flagN) const
	{
		static_assert(std::is_same_v<Flag1, Enum>, "Invalid Flag parameter!");
		return isSet(flag1) && isSet(flagN...);
	}

	template <typename Flag1, typename... FlagN>
	void set(Flag1 flag1, FlagN... flagN)
	{
		static_assert(std::is_same_v<Flag1, Enum>, "Invalid Flag parameter!");
		set(flag1);
		set(flagN...);
	}

	template <typename Flag1, typename... FlagN>
	void reset(Flag1 flag1, FlagN... flagN)
	{
		static_assert(std::is_same_v<Flag1, Enum>, "Invalid Flag parameter!");
		reset(flag1);
		reset(flagN...);
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
} // namespace le
