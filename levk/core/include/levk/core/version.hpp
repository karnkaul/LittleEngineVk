#pragma once
#include <levk/core/std_types.hpp>
#include <compare>
#include <string>

#if defined(__linux__)
#undef major
#undef minor
#undef patch
#endif

namespace le {
///
/// \brief Struct representing a semantic version
///
class Version {
  public:
	///
	/// \brief Parse string into version
	///
	Version(std::string_view serialised) noexcept;
	explicit constexpr Version(u32 major = 0, u32 minor = 0, u32 patch = 0, u32 tweak = 0) noexcept;

	constexpr u32 major() const noexcept { return m_major; }
	constexpr u32 minor() const noexcept { return m_minor; }
	constexpr u32 patch() const noexcept { return m_patch; }
	constexpr u32 tweak() const noexcept { return m_tweak; }
	std::string toString(bool full) const;

	///
	/// \brief Upgrade version if less than rhs
	///
	constexpr bool upgrade(Version const& rhs) noexcept;

	constexpr auto operator<=>(Version const&) const = default;

  private:
	u32 m_major;
	u32 m_minor;
	u32 m_patch;
	u32 m_tweak;
};

struct BuildVersion {
	Version version;
	std::string_view commitHash;

	std::string toString(bool full) const;
};

// impl

constexpr Version::Version(u32 major, u32 minor, u32 patch, u32 tweak) noexcept : m_major(major), m_minor(minor), m_patch(patch), m_tweak(tweak) {}
constexpr bool Version::upgrade(Version const& rhs) noexcept {
	if (*this < rhs) {
		*this = rhs;
		return true;
	}
	return false;
}
} // namespace le
