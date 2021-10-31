#pragma once
#include <core/maths.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <ktl/fixed_vector.hpp>
#include <window/types.hpp>

namespace le::window {
class Instance;
}

namespace le::input {
enum class Action : u8 { ePressed, eHeld, eReleased, eRepeated };
using Actions = ktl::enum_flags<Action, u8>;
constexpr Actions actions_main = Actions(Action::ePressed, Action::eHeld, Action::eReleased);
constexpr Actions actions_all = Actions(Action::ePressed, Action::eHeld, Action::eReleased, Action::eRepeated);

using Mod = window::Mod;
using Mods = ktl::enum_flags<Mod, u8, ktl::enum_trait_pot>;

enum class Focus : u8 { eUnchanged, eGained, eLost };
using Gamepad = window::Gamepad;
using Window = window::Instance;

using Key = window::Key;
using Axis = window::Axis;

struct KeyMods {
	Key key = Key::eUnknown;
	Mods mods{};

	constexpr KeyMods() = default;
	constexpr KeyMods(Key k) noexcept : key(k) {}
	constexpr KeyMods(Key k, Mod mod) noexcept : key(k) { mods.set(mod); }
	constexpr KeyMods(Key k, Mods m) noexcept : key(k), mods(m) {}
};

struct KeyState : KeyMods {
	using KeyMods::KeyMods;
	Actions actions;

	constexpr KeyState(Key k, Mods m, Actions mask) noexcept : KeyMods(k, m), actions(mask) {}
};

} // namespace le::input
