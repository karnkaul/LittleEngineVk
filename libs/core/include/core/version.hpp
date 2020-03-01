#pragma once
#include <string>
#include "core/std_types.hpp"

#if defined(__linux__)
#undef major
#undef minor
#undef patch
#endif

namespace le
{
struct Version
{
private:
	u32 mj;
	u32 mn;
	u32 pa;
	u32 tw;

public:
	Version(std::string_view serialised);
	explicit constexpr Version(u32 major = 0, u32 minor = 0, u32 patch = 0, u32 tweak = 0) noexcept
		: mj(major), mn(minor), pa(patch), tw(tweak)
	{
	}

public:
	u32 major() const;
	u32 minor() const;
	u32 patch() const;
	u32 tweak() const;
	std::string toString(bool bFull) const;

	bool upgrade(Version const& rhs);

	bool operator==(Version const& rhs);
	bool operator!=(Version const& rhs);
	bool operator<(Version const& rhs);
	bool operator<=(Version const& rhs);
	bool operator>(Version const& rhs);
	bool operator>=(Version const& rhs);
};
} // namespace le
