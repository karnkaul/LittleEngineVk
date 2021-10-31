#pragma once
#include <engine/input/types.hpp>
#include <glm/vec2.hpp>

namespace le::input {
struct Cursor {
	glm::vec2 position = {};
	glm::vec2 screenPos = {};
	glm::vec2 scroll = {};
};

struct State {
	ktl::fixed_vector<KeyState, 16> keys;
	Cursor cursor;
	Span<Gamepad const> gamepads;
	Span<u32 const> codepoints;
	Focus focus = Focus::eUnchanged;
	bool suspended = false;

	KeyState const* keyState(Key key) const noexcept;
	Mods mods(Key key) const noexcept;
	Actions actions(Key key) const noexcept;

	bool acted(Key key, Action action) const noexcept;
	bool pressed(Key key) const noexcept { return acted(key, Action::ePressed); }
	bool repeated(Key key) const noexcept { return acted(key, Action::eRepeated); }
	bool held(Key key) const noexcept { return acted(key, Action::eHeld); }
	bool released(Key key) const noexcept { return acted(key, Action::eReleased); }
};

// impl

inline KeyState const* State::keyState(Key key) const noexcept {
	if (key != Key{}) {
		for (auto const& k : keys) {
			if (k.key == key) { return &k; }
		}
	}
	return {};
}
inline Mods State::mods(Key key) const noexcept {
	Mods ret{};
	for (KeyState const& k : keys) {
		if (k.key == key) {
			ret = k.mods;
			break;
		}
	}
	return ret;
}
inline Actions State::actions(Key key) const noexcept {
	Actions ret{};
	for (KeyState const& k : keys) {
		if (k.key == key) {
			ret = k.actions;
			break;
		}
	}
	return ret;
}
inline bool State::acted(Key key, Action action) const noexcept {
	if (auto k = keyState(key); k && k->actions[action]) { return true; }
	return false;
}
} // namespace le::input
