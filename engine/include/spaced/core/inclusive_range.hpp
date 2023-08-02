#pragma once
#include <glm/common.hpp>

namespace spaced {
template <typename T>
struct InclusiveRange {
	T lo{};
	T hi{};

	constexpr auto clamp(T const& in) const -> T {
		using glm::clamp;
		return clamp(in, lo, hi);
	}
};
} // namespace spaced
