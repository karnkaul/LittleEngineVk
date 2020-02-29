#pragma once
#include <bitset>
#include <vector>
#include "core/std_types.hpp"

namespace le
{
template <utils::tName Enum>
struct TFlags
{
	std::bitset<size_t(Enum::COUNT_)> bits;

	constexpr TFlags() noexcept = default;
	explicit TFlags(std::string serialised);
	explicit TFlags(char const* szSerialised);

	bool isSet(Enum flag) const;
	void set(Enum flag, bool bValue);
	void set(std::initializer_list<Enum> flagList, bool bValue);
	void set(std::vector<Enum> const& flagList, bool bValue);
	void set(bool bValue);
};

template <utils::tName Enum>
TFlags<Enum>::TFlags(std::string serialised) : bits(std::move(serialised))
{
}

template <utils::tName Enum>
TFlags<Enum>::TFlags(char const* szSerialised) : bits(szSerialised)
{
}

template <utils::tName Enum>
bool TFlags<Enum>::isSet(Enum flag) const
{
	return bits[(size_t)flag];
}

template <utils::tName Enum>
void TFlags<Enum>::set(Enum flag, bool bValue)
{
	bits[(size_t)flag] = bValue;
}

template <utils::tName Enum>
void TFlags<Enum>::set(std::initializer_list<Enum> flagList, bool bValue)
{
	for (auto flag : flagList)
	{
		bits[(size_t)flag] = bValue;
	}
}

template <utils::tName Enum>
void TFlags<Enum>::set(std::vector<Enum> const& flagList, bool bValue)
{
	for (auto flag : flagList)
	{
		bits[(size_t)flag] = bValue;
	}
}

template <utils::tName Enum>
void TFlags<Enum>::set(bool bValue)
{
	bValue ? bits.set() : bits.reset();
}
} // namespace le
