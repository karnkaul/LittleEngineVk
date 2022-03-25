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

struct Driver::Parser : EventParser {
	Driver* driver{};
	State* state{};

	bool operator()(Event const& event) override {
		EXPECT(driver && state);
		switch (event.type()) {
		case Event::Type::eKey: {
			if (auto const& key = event.key(); key.key != GLFW_KEY_UNKNOWN) {
				driver->m_keyQueue.insert({Key(key.key), Action(key.action), Mod(key.mods)});
				return true;
			}
			return false;
		}
		case Event::Type::eMouseButton: {
			auto const& button = event.mouseButton();
			driver->m_keyQueue.insert({Key(button.button + int(Key::eMouseButton1)), Action(button.action), Mod(button.mods)});
			return true;
		}
		case Event::Type::eCursor: {
			driver->m_persistent.cursor = event.cursor();
			return true;
		}
		case Event::Type::eScroll: {
			state->cursor.scroll = event.scroll();
			return true;
		}
		case Event::Type::eText: {
			if (driver->m_transient.codepoints.has_space()) { driver->m_transient.codepoints.push_back(event.codepoint()); }
			return true;
		}
		case Event::Type::eFocusChange: {
			state->focus = event.focusGained() ? Focus::eGained : Focus::eLost;
			return true;
		}
		case Event::Type::eIconify: {
			driver->m_persistent.iconified = event.iconified();
			return false;
		}
		default: return false;
		}
	}
};

static void recurse(Event const& event, EventParser& root) {
	if (!root(event) && root.next) { recurse(event, *root.next); }
}

void Driver::parse(Span<Event const> events, EventParser& parser) {
	for (auto const& event : events) { recurse(event, parser); }
}

Driver::Driver() : m_keyDB(ktl::make_unique<KeyDB>()) {}

Frame Driver::update(In const& in, Viewport const& view) {
	Frame ret;
	auto& [st, sp] = ret;
	m_transient = {};
	Parser parser;
	parser.driver = this;
	parser.state = &st;
	parser.next = in.customParser;
	parse(in.events, parser);
	st.keyQueue = m_keyQueue.keys;
	st.keyDB = m_keyDB.get();
	m_keyQueue.collapse(*m_keyDB);
	st.cursor.screenPos = m_persistent.cursor;
	st.codepoints = m_transient.codepoints;
	st.iconified = m_persistent.iconified;
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
} // namespace le::input
