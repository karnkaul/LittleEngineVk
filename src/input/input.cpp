#include <core/utils/algo.hpp>
#include <engine/input/input.hpp>
#include <engine/render/viewport.hpp>
#include <window/desktop_instance.hpp>

namespace le {
namespace {
template <bool Erase, typename C>
C& op_equals(C& self, C const& rhs) {
	for (auto const& key : rhs.keys) {
		if constexpr (Erase) {
			self.erase(key.key);
		} else {
			self.insert(key.key, key.mods);
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

Input::Out Input::update(EventQueue queue, [[maybe_unused]] Viewport const& view, bool consume, [[maybe_unused]] DesktopInstance const* pDI) noexcept {
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
	s.cursor.screenPos = s.cursor.position = m_persistent.cursor;
	s.others = m_transient.others;
	s.text = m_transient.text;
	s.suspended = m_persistent.suspended;
#if defined(LEVK_DESKTOP)
	if (pDI) {
		m_transient.gamepads = pDI->activeGamepads();
		s.gamepads = m_transient.gamepads;
		auto const ifb = pDI->framebufferSize();
		auto const iwin = pDI->windowSize();
		glm::vec2 const win(f32(iwin.x), f32(iwin.y));
		if (view.scale < 1.0f) {
			s.cursor.position = (s.cursor.screenPos - win * view.topLeft.norm - view.topLeft.offset) / view.scale;
		}
		s.cursor.position *= (glm::vec2(f32(ifb.x), f32(ifb.y)) / win);
	}
#endif
	return ret;
}

bool Input::KeySet::insert(Key k, Mods const& mods) {
	if (k != Key::eUnknown) {
		for (auto& key : keys) {
			if (key.key == k) {
				key.mods.update(mods);
				return true;
			}
		}
		for (auto& key : keys) {
			if (key.key == Key::eUnknown) {
				key = {k, mods};
				return true;
			}
		}
	}
	return false;
}

bool Input::KeySet::erase(Key k) noexcept {
	for (auto& key : keys) {
		if (key.key == k) {
			key = {};
			return true;
		}
	}
	return false;
}

void Input::copy(KeySet const& in, kt::fixed_vector<KeyAct, 16>& out_keys, Input::Action action) {
	for (KeyMods const& key : in.keys) {
		if (key.key == Key::eUnknown) {
			continue;
		}
		bool found = false;
		for (auto& k : out_keys) {
			if (k.key == key.key) {
				found = true;
				k.t.update(action);
				k.mods = key.mods;
				break;
			}
		}
		if (!found && out_keys.has_space()) {
			KeyAct k{key, {}};
			k.t.update(action);
			out_keys.push_back(k);
		}
	}
}

bool Input::extract(Event const& event, State& out_state) noexcept {
	switch (event.type) {
	case Event::Type::eInput: {
		Event::Input const& input = event.payload.input;
		if (input.key != Key::eUnknown) {
			if (input.action == window::Action::ePress) {
				m_transient.pressed.insert(input.key, input.mods);
				m_persistent.held.erase(input.key);
			} else if (input.action == window::Action::eRelease) {
				m_transient.released.insert(input.key, input.mods);
				m_persistent.held.erase(input.key);
			}
			return true;
		}
		return false;
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
