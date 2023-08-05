#pragma once
#include <le/input/state.hpp>
#include <optional>
#include <vector>

namespace le::input {
struct Trigger {
	std::vector<int> keys{};
	std::vector<int> gamepad_buttons{};
	std::vector<Action> actions{Action::ePress};
	std::optional<std::size_t> gamepad_index{};

	[[nodiscard]] auto operator()(State const& state) const -> bool;
};
} // namespace le::input
