#pragma once
#include <deque>
#include <optional>
#include <glm/vec2.hpp>
#include <window/types.hpp>

namespace le::window {
struct Event {
	enum class Type : s8 {
		eFocus,
		eResize,
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
		char text;
		bool set;
	};

	Store payload;
	Type type;
};

struct EventQueue {
	std::deque<Event> m_events;

	std::optional<Event> pop() noexcept {
		if (!m_events.empty()) {
			Event ret = m_events.front();
			m_events.pop_front();
			return ret;
		}
		return std::nullopt;
	}
};
} // namespace le::window
