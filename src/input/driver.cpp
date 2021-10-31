#include <core/utils/algo.hpp>
#include <engine/input/driver.hpp>
#include <engine/input/space.hpp>
#include <engine/render/viewport.hpp>
#include <ktl/enum_flags/bitflags.hpp>
#include <window/instance.hpp>

namespace le::input {
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

Driver::Out Driver::update(In in, Viewport const& view, bool consume) noexcept {
	Out ret;
	auto& [f, q] = ret;
	auto& [st, sp] = f;
	m_transient.pressed -= m_transient.released;
	m_persistent.held += m_transient.pressed;
	m_transient = {};
	while (auto e = in.queue.pop()) {
		if (!extract(*e, st) || !consume) { q.m_events.push_back(*e); }
	}
	copy(m_transient.pressed, st.keys, Action::ePressed);
	copy(m_persistent.held, st.keys, Action::eHeld);
	copy(m_transient.released, st.keys, Action::eReleased);
	copy(m_transient.repeated, st.keys, Action::eRepeated);
	st.cursor.screenPos = m_persistent.cursor;
	st.codepoints = m_transient.codepoints;
	st.suspended = m_persistent.suspended;
	glm::vec2 wSize = {};
	if (in.win) {
		wSize = in.win->windowSize();
		m_transient.gamepads = in.win->activeGamepads();
		st.gamepads = m_transient.gamepads;
	}
	sp = Space::make(in.size.scene, in.size.swapchain, wSize, view, in.renderScale);
	st.cursor.position = sp.unproject(st.cursor.screenPos, false);
	return ret;
}

bool Driver::KeySet::insert(Key k, Mods const& mods) {
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

bool Driver::KeySet::erase(Key k) noexcept {
	for (auto& key : keys) {
		if (key.key == k) {
			key = {};
			return true;
		}
	}
	return false;
}

void Driver::copy(KeySet const& in, ktl::fixed_vector<KeyState, 16>& out_keys, Action action) {
	for (KeyMods const& key : in.keys) {
		if (key.key == Key::eUnknown) { continue; }
		bool found = false;
		for (auto& k : out_keys) {
			if (k.key == key.key) {
				found = true;
				k.actions.update(action);
				k.mods = key.mods;
				break;
			}
		}
		if (!found && out_keys.has_space()) {
			KeyState k(key.key, key.mods);
			k.actions.update(action);
			out_keys.push_back(k);
		}
	}
}

bool Driver::extract(Event const& event, State& out_state) noexcept {
	switch (event.type) {
	case Event::Type::eInput: {
		Event::Input const& input = event.payload.input;
		Mods const mods = Mod(input.mods);
		if (input.key != Key::eUnknown) {
			switch (input.action) {
			case window::Action::ePress: {
				m_transient.pressed.insert(input.key, mods);
				m_persistent.held.erase(input.key);
				break;
			}
			case window::Action::eRelease: {
				m_transient.released.insert(input.key, mods);
				m_persistent.held.erase(input.key);
				break;
			}
			case window::Action::eRepeat: {
				m_transient.repeated.insert(input.key, mods);
				break;
			}
			}
			return true;
		}
		return false;
	}
	case Event::Type::eCursor: {
		if (event.payload.cursor.id == 0) { m_persistent.cursor = event.payload.cursor; }
		return true;
	}
	case Event::Type::eScroll: {
		out_state.cursor.scroll = event.payload.cursor;
		return true;
	}
	case Event::Type::eText: {
		if (m_transient.codepoints.has_space()) { m_transient.codepoints.push_back(event.payload.codepoint); }
		return true;
	}
	case Event::Type::eFocus: {
		out_state.focus = event.payload.set ? Focus::eGained : Focus::eLost;
		return true;
	}
	case Event::Type::eSuspend: {
		m_persistent.suspended = event.payload.set;
		return false;
	}
	default: return false;
	}
}
} // namespace le::input
