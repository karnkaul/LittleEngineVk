#pragma once
#include <spaced/core/time.hpp>
#include <spaced/input/state.hpp>

namespace spaced::input {
struct KeyAxis {
	static constexpr Duration lerp_rate_v{0.2s};

	int lo{};
	int hi{};
	Duration lerp_rate{lerp_rate_v};

	float value{};

	[[nodiscard]] auto tick(State const& state, Duration dt) -> float;
};
} // namespace spaced::input
