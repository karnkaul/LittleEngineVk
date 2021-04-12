#pragma once
#include <core/maths.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>
#include <window/types.hpp>

namespace le::window {
class DesktopInstance;
}

namespace le::input {
enum class Action { ePressed = 1 << 0, eHeld = 1 << 1, eReleased = 1 << 2 };
using ActionMask = kt::uint_flags<>;
static constexpr ActionMask allActions = ActionMask::fill(maths::enumEnd(Action::eReleased));

enum class Focus { eUnchanged, eGained, eLost };
using Gamepad = window::Gamepad;
using DesktopInstance = window::DesktopInstance;

using Key = window::Key;
using Axis = window::Axis;
using Mod = window::Mod;
using Mods = window::Mods;

struct KeyMods {
	Key key = Key::eUnknown;
	Mods mods{};

	constexpr KeyMods() = default;
	constexpr KeyMods(Key k) noexcept;
	constexpr KeyMods(Key k, Mod mod) noexcept;
	constexpr KeyMods(Key k, Mods m) noexcept;
};

template <typename T>
struct KeyCombo : KeyMods {
	T t{};
};
using KeyAct = KeyCombo<ActionMask>;

// impl

constexpr KeyMods::KeyMods(Key k) noexcept : key(k) {
}
constexpr KeyMods::KeyMods(Key k, Mod mod) noexcept : key(k) {
	mods.add(mod);
}
constexpr KeyMods::KeyMods(Key k, Mods m) noexcept : key(k), mods(m) {
}
} // namespace le::input
