#pragma once
#include <engine/input/types.hpp>
#include <glm/vec2.hpp>
#include <ktl/result.hpp>
#include <window/event_queue.hpp>

namespace le::input {
using Event = window::Event;
using EventQueue = window::EventQueue;

struct Cursor {
	glm::vec2 position = {};
	glm::vec2 screenPos = {};
	glm::vec2 scroll = {};
};

struct State {
	template <typename T>
	using List = std::initializer_list<T>;
	template <typename T>
	using Res = ktl::result<T, void>;

	ktl::fixed_vector<KeyAct, 16> keys;
	Cursor cursor;
	Span<Gamepad const> gamepads;
	Span<Event::Cursor const> others;
	Span<char const> text;
	Focus focus = Focus::eUnchanged;
	bool suspended = false;

	KeyAct const& keyMask(Key key) const noexcept;
	Mods mods(Key key) const noexcept;
	Actions actions(Key key) const noexcept;

	Res<KeyMods> acted(Key) const noexcept;
	Res<KeyMods> acted(Key key, Action action) const noexcept;
	Res<KeyMods> pressed(Key key) const noexcept;
	Res<KeyMods> held(Key key) const noexcept;
	Res<KeyMods> released(Key key) const noexcept;

	bool any(List<Key> keys, Actions mask = actions_all) const noexcept;
	bool all(List<Key> keys, Actions mask = actions_all) const noexcept;
};

// impl

inline KeyAct const& State::keyMask(Key key) const noexcept {
	static constexpr KeyAct blank{};
	if (key == blank.key) { return blank; }
	for (auto const& k : keys) {
		if (k.key == key) { return k; }
	}
	return blank;
}
inline Mods State::mods(Key key) const noexcept {
	Mods ret{};
	for (KeyAct const& k : keys) {
		if (k.key == key) {
			ret = k.mods;
			break;
		}
	}
	return ret;
}
inline Actions State::actions(Key key) const noexcept {
	Actions ret{};
	for (KeyAct const& k : keys) {
		if (k.key == key) {
			ret = k.t;
			break;
		}
	}
	return ret;
}
inline State::Res<KeyMods> State::acted(Key key) const noexcept {
	KeyAct const k = keyMask(key);
	if (k.key != Key::eUnknown) { return k; }
	return ktl::null_result;
}
inline State::Res<KeyMods> State::acted(Key key, Action action) const noexcept {
	KeyAct const k = keyMask(key);
	if (k.key != Key::eUnknown && k.t[action]) { return k; }
	return ktl::null_result;
}
inline State::Res<KeyMods> State::pressed(Key key) const noexcept { return acted(key, Action::ePressed); }
inline State::Res<KeyMods> State::held(Key key) const noexcept { return acted(key, Action::eHeld); }
inline State::Res<KeyMods> State::released(Key key) const noexcept { return acted(key, Action::eReleased); }
inline bool State::any(List<Key> keys, Actions mask) const noexcept {
	for (Key const k : keys) {
		if (actions(k).any(mask)) { return true; }
	}
	return false;
}
inline bool State::all(List<Key> keys, Actions mask) const noexcept {
	for (Key const k : keys) {
		if (!actions(k).any(mask)) { return false; }
	}
	return keys.size() > 0;
}
} // namespace le::input
