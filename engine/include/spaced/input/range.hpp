#pragma once
// #include <levk/window/gamepad.hpp>
#include <spaced/input/key_axis.hpp>
#include <optional>
#include <vector>

namespace spaced::input {
struct Range {
	std::vector<KeyAxis> key_axes{};
	std::vector<int> gamepad_axes{};
	std::optional<std::size_t> gamepad_index{};

	[[nodiscard]] auto operator()(State const& state, Duration dt) -> float;
};
} // namespace spaced::input
