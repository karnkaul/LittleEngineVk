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
	major_ = parse(tokens, 0);
	minor_ = parse(tokens, 1);
	patch_ = parse(tokens, 2);
	tweak_ = parse(tokens, 3);
}

std::string Version::toString(bool full) const {
	std::stringstream ret;
	ret << major_ << "." << minor_;
	if (tweak_ > 0 || patch_ > 0 || full) { ret << "." << patch_; }
	if (tweak_ > 0 || full) { ret << "." << tweak_; }
	return ret.str();
}
} // namespace le
