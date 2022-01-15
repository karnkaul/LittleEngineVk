#pragma once
#include <glm/vec2.hpp>
#include <levk/engine/input/types.hpp>

namespace le::input {
struct Cursor {
	glm::vec2 position = {};
	glm::vec2 screenPos = {};
	glm::vec2 scroll = {};
};

struct State {
	Cursor cursor;
	Span<Gamepad const> gamepads;
	Span<KeyEvent const> keyQueue;
	Span<u32 const> codepoints;
	KeyDB const* keyDB{};
	Focus focus = Focus::eUnchanged;
	bool iconified = false;

	static Mods const* getMods(std::optional<Mods> const& m) noexcept { return m ? &*m : nullptr; }
	Mods const* mods(Key key, Action action) const noexcept { return keyDB ? getMods(keyDB->mods[key][action]) : nullptr; }
	Mods const* pressed(Key key) const noexcept { return mods(key, Action::ePress); }
	Mods const* released(Key key) const noexcept { return mods(key, Action::eRelease); }
	Mods const* repeated(Key key) const noexcept { return mods(key, Action::eRepeat); }
	Mods const* pressOrRepeat(Key key) const noexcept;
	bool held(Key key) const noexcept { return keyDB && keyDB->held[key]; }
};

// impl

inline Mods const* State::pressOrRepeat(Key key) const noexcept {
	if (auto const ret = pressed(key)) { return ret; }
	return repeated(key);
}
} // namespace le::input
