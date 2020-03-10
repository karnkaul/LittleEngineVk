#pragma once
#include <bitset>
#include <initializer_list>
#include <string>
#include <type_traits>
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
		return bits.test((size_t)flag);
	}

	void set(Enum flag)
	{
		bits.set((size_t)flag);
	}

	void reset(Enum flag)
	{
		bits.reset((size_t)flag);
	}
	void set(std::initializer_list<Enum> flagList)
	{
		for (auto flag : flagList)
		{
			bits.set((size_t)flag);
		}
	}
	void reset(std::initializer_list<Enum> flagList)
	{
		for (auto flag : flagList)
		{
			bits.reset((size_t)flag);
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
};
} // namespace le
