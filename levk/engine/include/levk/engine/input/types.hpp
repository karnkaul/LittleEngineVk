#pragma once
#include <core/maths.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <ktl/fixed_vector.hpp>
#include <levk/window/types.hpp>

namespace le::window {
class Instance;
}

namespace le::input {
using Action = window::Action;
using Mod = window::Mod;
using Mods = ktl::enum_flags<Mod, u8, ktl::enum_trait_pot>;

enum class Focus : u8 { eUnchanged, eGained, eLost };
using Gamepad = window::Gamepad;
using Window = window::Instance;

using Key = window::Key;
using Axis = window::Axis;

struct KeyEvent {
	Key key{};
	Action action{};
	Mods mods;
};

template <typename T>
using TKeyDB = EnumArray<Key, T, 512>;

struct KeyDB {
	TKeyDB<EnumArray<Action, std::optional<Mods>, 3>> mods;
	TKeyDB<bool> held;
};
} // namespace le::input
