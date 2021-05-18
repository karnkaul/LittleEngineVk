#pragma once
#include <compare>
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
	constexpr u32 major() const noexcept { return major_; }
	constexpr u32 minor() const noexcept { return minor_; }
	constexpr u32 patch() const noexcept { return patch_; }
	constexpr u32 tweak() const noexcept { return tweak_; }
	std::string toString(bool full) const;

	///
	/// \brief Upgrade version if less than rhs
	///
	constexpr bool upgrade(Version const& rhs) noexcept;

	constexpr auto operator<=>(Version const&) const = default;

  private:
	u32 major_;
	u32 minor_;
	u32 patch_;
	u32 tweak_;
};

// impl

constexpr Version::Version(u32 major, u32 minor, u32 patch, u32 tweak) noexcept : major_(major), minor_(minor), patch_(patch), tweak_(tweak) {}
constexpr bool Version::upgrade(Version const& rhs) noexcept {
	if (*this < rhs) {
		*this = rhs;
		return true;
	}
	return false;
}
} // namespace le
