#pragma once
#include <core/maths.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <ktl/fixed_vector.hpp>
#include <window/types.hpp>

namespace le::window {
class Instance;
}

namespace le::input {
enum class Action : u8 { ePressed, eHeld, eReleased };
using Actions = ktl::enum_flags<Action, u8>;
constexpr Actions actions_all = Actions(Action::ePressed, Action::eHeld, Action::eReleased);

enum class Focus { eUnchanged, eGained, eLost };
using Gamepad = window::Gamepad;
using Window = window::Instance;

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
using KeyAct = KeyCombo<Actions>;

// impl

constexpr KeyMods::KeyMods(Key k) noexcept : key(k) {}
constexpr KeyMods::KeyMods(Key k, Mod mod) noexcept : key(k) { mods.set(mod); }
constexpr KeyMods::KeyMods(Key k, Mods m) noexcept : key(k), mods(m) {}
} // namespace le::input
