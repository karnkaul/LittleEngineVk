#pragma once
#include <core/std_types.hpp>

namespace le {
namespace graphics {
class Pipeline;
}

struct DrawLayer {
	graphics::Pipeline* pipeline{};
	s64 order = 0;

	bool operator==(DrawLayer const& rhs) const noexcept = default;
	auto operator<=>(DrawLayer const& rhs) const noexcept { return order <=> rhs.order; }
};
} // namespace le
