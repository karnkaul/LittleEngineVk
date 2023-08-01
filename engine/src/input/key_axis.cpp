#include <spaced/input/key_axis.hpp>

namespace spaced::input {
auto KeyAxis::tick(State const& state, Duration const dt) -> float {
	static constexpr auto unit = [](float const in) { return in > 0.0f ? 1.0f : -1.0f; };
	auto raw_input = float{};
	if (state.keyboard.at(lo) == Action::eHold) { raw_input -= 1.0f; }
	if (state.keyboard.at(hi) == Action::eHold) { raw_input += 1.0f; }

	if (raw_input == 0.0f && std::abs(value) < 0.1f) {
		value = 0.0f;
	} else {
		auto const dv = raw_input == 0.0f ? -unit(value) : raw_input;
		value += dv * dt / lerp_rate;
	}
	value = std::clamp(value, -1.0f, 1.0f);
	return value;
}
} // namespace spaced::input
