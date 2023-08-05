#include <le/input/range.hpp>

namespace le::input {
auto Range::operator()(State const& state, Duration const dt) -> float {
	auto ret = float{};
	for (auto& axis : key_axes) { ret += axis.tick(state, dt); }
	if (!gamepad_axes.empty()) {
		auto const id = gamepad_index.value_or(state.last_engaged_gamepad_index);
		auto const& gamepad = state.gamepads.at(id);
		for (auto const axis : gamepad_axes) { ret += gamepad.axes.at(static_cast<std::size_t>(axis)); }
	}
	return ret;
}
} // namespace le::input
