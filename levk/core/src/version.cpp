#include <levk/core/span.hpp>
#include <levk/core/utils/string.hpp>
#include <levk/core/version.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace le {
namespace {
u32 parse(Span<std::string_view const> const& vec, std::size_t idx) noexcept { return (vec.size() > idx) ? utils::to<u32>(vec[idx], 0) : 0; }
} // namespace

Version::Version(std::string_view serialised) noexcept {
	auto const tokens = utils::tokenise<4>(serialised, '.');
	m_major = parse(tokens, 0);
	m_minor = parse(tokens, 1);
	m_patch = parse(tokens, 2);
	m_tweak = parse(tokens, 3);
}

std::string Version::toString(bool full) const {
	std::stringstream ret;
	ret << m_major << "." << m_minor;
	if (m_tweak > 0 || m_patch > 0 || full) { ret << "." << m_patch; }
	if (m_tweak > 0 || full) { ret << "." << m_tweak; }
	return ret.str();
}

std::string BuildVersion::toString(bool full) const {
	auto ret = version.toString(full);
	if (full) {
		ret += '-';
		ret += commitHash;
	}
	return ret;
}
} // namespace le
