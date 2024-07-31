#pragma once
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <array>
#include <optional>

namespace levk::input {
inline constexpr std::size_t max_keys_v{512};
inline constexpr std::size_t max_mouse_buttons_v{16};

enum class Action : std::int8_t { eNone, ePress, eHold, eRelease };

template <std::size_t Size>
constexpr auto to_safe_index(int const index) -> std::optional<std::size_t> {
	if (index < 0) { return {}; }
	auto const idx = static_cast<std::size_t>(index);
	if (idx > Size) { return {}; }
	return idx;
}

template <std::size_t Size>
struct ActionTable {
	std::array<Action, Size> table{};

	[[nodiscard]] constexpr auto at(int const index) const -> Action {
		auto const idx = to_safe_index<Size>(index);
		if (!idx) { return Action::eNone; }
		return table[*idx]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
	}

	[[nodiscard]] constexpr auto is_pressed(int const index) const -> bool { return at(index) == Action::ePress; }
	[[nodiscard]] constexpr auto is_held(int const index) const -> bool { return at(index) == Action::eHold; }
	[[nodiscard]] constexpr auto is_released(int const index) const -> bool { return at(index) == Action::eRelease; }

	[[nodiscard]] constexpr auto operator[](int const index) const -> Action { return at(index); }
};

struct State {
	ActionTable<max_keys_v> keys{};
	ActionTable<max_mouse_buttons_v> mouse_buttons{};
	glm::vec2 cursor_position{};
	glm::vec2 mouse_scroll{};
};
} // namespace levk::input
