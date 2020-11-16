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
		eScroll
	};
	using Focus = bool;
	using Suspend = bool;
	struct Resize final : glm::ivec2 {
		bool bFramebuffer;
	};
	struct Input final {
		Key key;
		Action action;
		Mods mods;
	};
	struct Text final {
		char c;
	};
	struct Cursor final : glm::tvec2<f64> {
		s32 id;
	};
	union Store {
		bool bSet;
		Resize resize;
		Input input;
		Text text;
		Cursor cursor;
	};

	Store payload;
	Type type;
};

struct EventQueue {
	std::optional<Event> pop();

	std::deque<Event> m_events;
};

inline std::optional<Event> EventQueue::pop() {
	if (!m_events.empty()) {
		Event ret = m_events.front();
		m_events.pop_front();
		return ret;
	}
	return std::nullopt;
}
} // namespace le::window
