#include <le/input/trigger.hpp>
#include <algorithm>

namespace le::input {
auto Trigger::operator()(State const& state) const -> bool {
	for (auto const action : actions) {
		if (std::ranges::any_of(keys, [&state, action](std::size_t const key) { return state.keyboard.at(key) == action; })) { return true; }
		if (!gamepad_buttons.empty()) {
			auto const id = gamepad_index.value_or(state.last_engaged_gamepad_index);
			auto const& gamepad = state.gamepads.at(id);
			if (std::ranges::any_of(gamepad_buttons, [action, &gamepad](std::size_t const key) { return gamepad.buttons.at(key) == action; })) { return true; }
		}
	}
	return false;
}
} // namespace le::input
