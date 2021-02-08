#include <sstream>
#include <string>
#include <vector>
#include <core/span.hpp>
#include <core/utils.hpp>
#include <core/version.hpp>

namespace le {
namespace {
u32 parse(View<std::string_view> const& vec, std::size_t idx) {
	return (vec.size() > idx) ? u32(utils::strings::toS32(vec[idx], 0)) : 0;
}
} // namespace

Version::Version(std::string_view serialised) {
	auto const tokens = utils::strings::tokenise<4>(serialised, '.');
	mj = parse(tokens, 0);
	mn = parse(tokens, 1);
	pa = parse(tokens, 2);
	tw = parse(tokens, 3);
}

u32 Version::major() const noexcept {
	return mj;
}

u32 Version::minor() const noexcept {
	return mn;
}

u32 Version::patch() const noexcept {
	return pa;
}

u32 Version::tweak() const noexcept {
	return tw;
}

std::string Version::toString(bool bFull) const {
	std::stringstream ret;
	ret << mj << "." << mn;
	if (tw > 0 || pa > 0 || bFull) {
		ret << "." << pa;
	}
	if (tw > 0 || bFull) {
		ret << "." << tw;
	}
	return ret.str();
}

bool Version::upgrade(Version const& rhs) noexcept {
	if (*this < rhs) {
		*this = rhs;
		return true;
	}
	return false;
}

bool Version::operator==(Version const& rhs) noexcept {
	return mj == rhs.mj && mn == rhs.mn && pa == rhs.pa && tw == rhs.tw;
}

bool Version::operator!=(Version const& rhs) noexcept {
	return !(*this == rhs);
}

bool Version::operator<(Version const& rhs) noexcept {
	return (mj < rhs.mj) || (mj == rhs.mj && mn < rhs.mn) || (mj == rhs.mj && mn == rhs.mn && pa < rhs.pa) ||
		   (mj == rhs.mj && mn == rhs.mn && pa == rhs.pa && tw < rhs.tw);
}

bool Version::operator<=(Version const& rhs) noexcept {
	return (*this == rhs) || (*this < rhs);
}

bool Version::operator>(Version const& rhs) noexcept {
	return (mj > rhs.mj) || (mj == rhs.mj && mn > rhs.mn) || (mj == rhs.mj && mn == rhs.mn && pa > rhs.pa) ||
		   (mj == rhs.mj && mn == rhs.mn && pa == rhs.pa && tw > rhs.tw);
}

bool Version::operator>=(Version const& rhs) noexcept {
	return (*this == rhs) || (*this > rhs);
}
} // namespace le
