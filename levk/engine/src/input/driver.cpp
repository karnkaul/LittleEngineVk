#include <GLFW/glfw3.h>
#include <ktl/enum_flags/bitflags.hpp>
#include <levk/core/utils/algo.hpp>
#include <levk/engine/input/driver.hpp>
#include <levk/engine/input/space.hpp>
#include <levk/engine/render/viewport.hpp>
#include <levk/window/window.hpp>

namespace le::input {
void Driver::KeyQueue::insert(KeyEvent event) noexcept {
	if (keys.has_space()) { keys.push_back(event); }
}

void Driver::KeyQueue::collapse(KeyDB& out) noexcept {
	out.mods = {};
	for (auto const& key : keys) {
		if (static_cast<std::size_t>(key.key) < sizeof(TKeyDB<Key>::arr)) {
			out.mods[key.key][key.action] = key.mods;
			out.held[key.key] = key.action == Action::ePress || key.action == Action::eRepeat;
		}
	}
	keys.clear();
}

Driver::Driver() : m_keyDB(std::make_unique<KeyDB>()) {}

Driver::Out Driver::update(In in, Viewport const& view, bool consume) {
	Out ret;
	auto& [f, q] = ret;
	auto& [st, sp] = f;
	m_transient = {};
	q = parse(*this, in.queue, st);
	if (!consume) { q = in.queue; }
	st.keyQueue = m_keyQueue.keys;
	st.keyDB = m_keyDB.get();
	m_keyQueue.collapse(*m_keyDB);
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

bool Driver::operator()(Event const& event, State& out_state) {
	switch (event.type()) {
	case Event::Type::eKey: {
		if (auto const& key = event.key(); key.key != GLFW_KEY_UNKNOWN) {
			m_keyQueue.insert({Key(key.key), Action(key.action), Mod(key.mods)});
			return true;
		}
		return false;
	}
	case Event::Type::eMouseButton: {
		auto const& button = event.mouseButton();
		m_keyQueue.insert({Key(button.button + int(Key::eMouseButton1)), Action(button.action), Mod(button.mods)});
		return true;
	}
	case Event::Type::eCursor: {
		m_persistent.cursor = event.cursor();
		return true;
	}
	case Event::Type::eScroll: {
		out_state.cursor.scroll = event.scroll();
		return true;
	}
	case Event::Type::eText: {
		if (m_transient.codepoints.has_space()) { m_transient.codepoints.push_back(event.codepoint()); }
		return true;
	}
	case Event::Type::eFocusChange: {
		out_state.focus = event.focusGained() ? Focus::eGained : Focus::eLost;
		return true;
	}
	case Event::Type::eIconify: {
		m_persistent.suspended = event.iconified();
		return false;
	}
	default: return false;
	}
}
} // namespace le::input
