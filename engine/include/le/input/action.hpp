#pragma once
#include <string_view>

namespace le::input {
enum class Action { eNone, ePress, eHold, eRepeat, eRelease };

constexpr auto to_str(input::Action const action) -> std::string_view {
	switch (action) {
	case input::Action::eNone: return "none";
	case input::Action::ePress: return "press";
	case input::Action::eHold: return "hold";
	case input::Action::eRelease: return "release";
	default: return "unexpected";
	}
}
} // namespace le::input
