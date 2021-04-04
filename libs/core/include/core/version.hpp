#pragma once
#include <string>
#include <core/std_types.hpp>

#if defined(__linux__)
#undef major
#undef minor
#undef patch
#endif

namespace le {
///
/// \brief Struct representing a semantic version
///
struct Version {
  public:
	///
	/// \brief Parse string into version
	///
	Version(std::string_view serialised) noexcept;
	explicit constexpr Version(u32 major = 0, u32 minor = 0, u32 patch = 0, u32 tweak = 0) noexcept;

  public:
	constexpr u32 major() const noexcept;
	constexpr u32 minor() const noexcept;
	constexpr u32 patch() const noexcept;
	constexpr u32 tweak() const noexcept;
	std::string toString(bool bFull) const;

	///
	/// \brief Upgrade version if less than rhs
	///
	constexpr bool upgrade(Version const& rhs) noexcept;

	constexpr friend bool operator==(Version const& lhs, Version const& rhs) noexcept;
	constexpr friend bool operator!=(Version const& lhs, Version const& rhs) noexcept;
	constexpr friend bool operator<(Version const& lhs, Version const& rhs) noexcept;
	constexpr friend bool operator<=(Version const& lhs, Version const& rhs) noexcept;
	constexpr friend bool operator>(Version const& lhs, Version const& rhs) noexcept;
	constexpr friend bool operator>=(Version const& lhs, Version const& rhs) noexcept;

  private:
	u32 mj;
	u32 mn;
	u32 pa;
	u32 tw;
};

// impl

constexpr Version::Version(u32 major, u32 minor, u32 patch, u32 tweak) noexcept : mj(major), mn(minor), pa(patch), tw(tweak) {
}
constexpr u32 Version::major() const noexcept {
	return mj;
}
constexpr u32 Version::minor() const noexcept {
	return mn;
}
constexpr u32 Version::patch() const noexcept {
	return pa;
}
constexpr u32 Version::tweak() const noexcept {
	return tw;
}
constexpr bool Version::upgrade(Version const& rhs) noexcept {
	if (*this < rhs) {
		*this = rhs;
		return true;
	}
	return false;
}
constexpr bool operator==(Version const& lhs, Version const& rhs) noexcept {
	return lhs.mj == rhs.mj && lhs.mn == rhs.mn && lhs.pa == rhs.pa && lhs.tw == rhs.tw;
}
constexpr bool operator!=(Version const& lhs, Version const& rhs) noexcept {
	return !(lhs == rhs);
}
constexpr bool operator<(Version const& lhs, Version const& rhs) noexcept {
	return (lhs.mj < rhs.mj) || (lhs.mj == rhs.mj && lhs.mn < rhs.mn) || (lhs.mj == rhs.mj && lhs.mn == rhs.mn && lhs.pa < rhs.pa) ||
		   (lhs.mj == rhs.mj && lhs.mn == rhs.mn && lhs.pa == rhs.pa && lhs.tw < rhs.tw);
}
constexpr bool operator<=(Version const& lhs, Version const& rhs) noexcept {
	return (lhs == rhs) || (lhs < rhs);
}
constexpr bool operator>(Version const& lhs, Version const& rhs) noexcept {
	return (lhs.mj > rhs.mj) || (lhs.mj == rhs.mj && lhs.mn > rhs.mn) || (lhs.mj == rhs.mj && lhs.mn == rhs.mn && lhs.pa > rhs.pa) ||
		   (lhs.mj == rhs.mj && lhs.mn == rhs.mn && lhs.pa == rhs.pa && lhs.tw > rhs.tw);
}
constexpr bool operator>=(Version const& lhs, Version const& rhs) noexcept {
	return (lhs == rhs) || (lhs > rhs);
}
} // namespace le
