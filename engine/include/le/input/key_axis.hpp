#pragma once
#include <le/core/time.hpp>
#include <le/input/state.hpp>

namespace le::input {
struct KeyAxis {
	static constexpr Duration lerp_rate_v{0.2s};

	int lo{};
	int hi{};
	Duration lerp_rate{lerp_rate_v};

	float value{};

	[[nodiscard]] auto tick(State const& state, Duration dt) -> float;
};
} // namespace le::input
