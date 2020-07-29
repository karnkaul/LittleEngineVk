#pragma once
#include <string>
#include <core/std_types.hpp>

#if defined(__linux__)
#undef major
#undef minor
#undef patch
#endif

namespace le
{
///
/// \brief Struct representing a semantic version
///
struct Version
{
private:
	u32 mj;
	u32 mn;
	u32 pa;
	u32 tw;

public:
	///
	/// \brief Parse string into version
	///
	Version(std::string_view serialised);
	explicit constexpr Version(u32 major = 0, u32 minor = 0, u32 patch = 0, u32 tweak = 0) noexcept : mj(major), mn(minor), pa(patch), tw(tweak) {}

public:
	u32 major() const noexcept;
	u32 minor() const noexcept;
	u32 patch() const noexcept;
	u32 tweak() const noexcept;
	std::string toString(bool bFull) const;

	///
	/// \brief Upgrade version if less than rhs
	///
	bool upgrade(Version const& rhs) noexcept;

	bool operator==(Version const& rhs) noexcept;
	bool operator!=(Version const& rhs) noexcept;
	bool operator<(Version const& rhs) noexcept;
	bool operator<=(Version const& rhs) noexcept;
	bool operator>(Version const& rhs) noexcept;
	bool operator>=(Version const& rhs) noexcept;
};
} // namespace le
