#pragma once
#include <glm/vec2.hpp>
#include <array>
#include <vector>

namespace spaced::input {
enum class Action { eNone, ePress, eHold, eRepeat, eRelease };

struct State {
	enum Property : std::uint32_t {
		eWindowSize,
		eFramebufferSize,
		eCursorPosition,
		eFocus,
	};

	static constexpr std::size_t max_keys_v{512};
	static constexpr std::size_t max_mouse_buttons_v{8};

	std::array<Action, max_keys_v> keyboard{};				 // key: GLFW_KEY_*
	std::array<Action, max_mouse_buttons_v> mouse_buttons{}; // key: GLFW_MOUSE_BUTTON_*
	std::vector<std::uint32_t> characters{};

	glm::uvec2 window_size{};
	glm::uvec2 framebuffer_size{};
	glm::vec2 cursor_position{};

	bool in_focus{};
	bool shutting_down{};

	std::uint32_t changed{};
};
} // namespace spaced::input
