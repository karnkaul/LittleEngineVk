#pragma once
#include <spaced/input/state.hpp>
#include <vector>

namespace spaced::input {
struct Trigger {
	std::vector<int> keys{};
	std::vector<Action> actions{Action::ePress};

	[[nodiscard]] auto operator()(State const& state) const -> bool;
};
} // namespace spaced::input
