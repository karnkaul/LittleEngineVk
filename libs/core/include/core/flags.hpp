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
	explicit TFlags(std::string serialised) : bits(std::move(serialised)) {}

	explicit TFlags(char const* szSerialised) : bits(szSerialised) {}

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
};
} // namespace le
