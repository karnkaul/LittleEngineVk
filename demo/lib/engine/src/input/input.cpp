#include <core/utils/algo.hpp>
#include <engine/input/input.hpp>
#include <window/desktop_instance.hpp>

namespace le {
using namespace window;

namespace {
void copy(std::unordered_set<window::Key> const& in, kt::fixed_vector<Input::Key, 16>& out_keys, Input::Action action) {
	for (window::Key const key : in) {
		bool found = false;
		for (Input::Key& k : out_keys) {
			if (k.key == key) {
				found = true;
				k.actions.set(action);
				break;
			}
		}
		if (!found && out_keys.has_space()) {
			out_keys.push_back({key, action});
		}
	}
}

template <bool Erase, typename C>
C& op_equals(C& self, C const& rhs) {
	for (auto const& key : rhs) {
		if constexpr (Erase) {
			self.erase(key);
		} else {
			self.insert(key);
		}
	}
	return self;
}

template <typename C>
C& operator+=(C& self, C const& rhs) {
	return op_equals<false>(self, rhs);
}

template <typename C>
C& operator-=(C& self, C const& rhs) {
	return op_equals<true>(self, rhs);
}
} // namespace

Input::Out Input::update(EventQueue queue, bool consume, window::DesktopInstance const* pDI) noexcept {
	Out ret;
	auto& [s, q] = ret;
	m_transient.pressed -= m_transient.released;
	m_persistent.held += m_transient.pressed;
	m_transient = {};
	while (auto e = queue.pop()) {
		if (!extract(*e, s) || !consume) {
			q.m_events.push_back(*e);
		}
	}
	copy(m_transient.pressed, s.keys, Action::ePressed);
	copy(m_persistent.held, s.keys, Action::eHeld);
	copy(m_transient.released, s.keys, Action::eReleased);
	s.cursor.position = m_persistent.cursor;
	s.others = m_transient.others;
	s.text = m_transient.text;
	s.suspended = m_persistent.suspended;
	if (pDI) {
		m_transient.gamepads = pDI->activeGamepads();
		s.gamepads = m_transient.gamepads;
	}
	return ret;
}

bool Input::extract(Event const& event, State& out_state) noexcept {
	switch (event.type) {
	case Event::Type::eInput: {
		window::Event::Input const& input = event.payload.input;
		if (input.action == window::Action::ePress) {
			m_transient.pressed.insert(input.key);
			m_persistent.held.erase(input.key);
		} else if (input.action == window::Action::eRelease) {
			m_transient.released.insert(input.key);
			m_persistent.held.erase(input.key);
		}
		return true;
	}
	case Event::Type::eCursor: {
		Event::Cursor const& cursor = event.payload.cursor;
		if (cursor.id == 0) {
			m_persistent.cursor = cursor;
		} else if (m_transient.others.has_space()) {
			m_transient.others.push_back(cursor);
		}
		return true;
	}
	case Event::Type::eScroll: {
		out_state.cursor.scroll = event.payload.cursor;
		return true;
	}
	case Event::Type::eText: {
		if (m_transient.text.has_space()) {
			m_transient.text.push_back(event.payload.text.c);
		}
		return true;
	}
	case Event::Type::eFocus: {
		out_state.focus = event.payload.bSet ? Input::Focus::eGained : Input::Focus::eLost;
		return true;
	}
	case Event::Type::eSuspend: {
		m_persistent.suspended = event.payload.bSet;
		return false;
	}
	default:
		return false;
	}
}
} // namespace le
