#pragma once
#include <glm/vec2.hpp>
#include <levk/core/codepoint.hpp>
#include <levk/window/types.hpp>
#include <optional>

namespace le::window {
struct Event {
	enum class Type : s8 {
		eFocus,
		eResize,
		eMaximize,
		eSuspend,
		eClose,
		eInput,
		eText,
		eCursor,
		eScroll,
	};
	struct Resize : glm::uvec2 {
		bool framebuffer;
	};
	struct Input {
		Key key;
		Action action;
		Mods mods;
		s32 scancode;
	};
	struct Cursor : glm::tvec2<f64> {
		s32 id;
	};
	union Store {
		Resize resize;
		Input input;
		Cursor cursor;
		Codepoint::type codepoint;
		bool set;
	};

	Store payload;
	Type type;
};

using EventQueue = std::vector<Event>;
} // namespace le::window
