#include <spaced/input/range.hpp>

namespace spaced::input {
auto Range::operator()(State const& state, Duration const dt) -> float {
	auto ret = float{};
	for (auto& axis : key_axes) { ret += axis.tick(state, dt); }
	if (!gamepad_axes.empty()) {
		// TODO: gamepad
		// assert(!gamepad_index || *gamepad_index < state.gamepads.size());
		// auto const& gamepad = state.gamepads[gamepad_index.value_or(state.last_engaged_gamepad_index)];
		// for (auto const axis : gamepad_axes) { ret += gamepad.axes.value(axis); }
	}
	return ret;
}
} // namespace spaced::input
