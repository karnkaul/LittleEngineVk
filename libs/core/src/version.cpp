#include <sstream>
#include <string>
#include <vector>
#include <core/span.hpp>
#include <core/utils/string.hpp>
#include <core/version.hpp>

namespace le {
namespace {
u32 parse(View<std::string_view> const& vec, std::size_t idx) noexcept { return (vec.size() > idx) ? utils::to<u32>(vec[idx], 0) : 0; }
} // namespace

Version::Version(std::string_view serialised) noexcept {
	auto const tokens = utils::tokenise<4>(serialised, '.');
	mj = parse(tokens, 0);
	mn = parse(tokens, 1);
	pa = parse(tokens, 2);
	tw = parse(tokens, 3);
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
} // namespace le
