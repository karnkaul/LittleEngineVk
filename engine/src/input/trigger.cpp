#include <spaced/input/trigger.hpp>
#include <algorithm>

namespace spaced::input {
auto Trigger::operator()(State const& state) const -> bool {
	for (auto const action : actions) {
		if (std::ranges::any_of(keys, [&state, action = action](int const key) { return state.keyboard.at(key) == action; })) { return true; }
	}
	return false;
}
} // namespace spaced::input
