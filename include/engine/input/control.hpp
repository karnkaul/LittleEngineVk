#pragma once
#include <variant>
#include <engine/input/types.hpp>

namespace le::input {
struct State;

struct AxisRange {
	std::size_t padID = 0;
	Axis axis = {};
	bool invert = false;
};
struct KeyRange {
	KeyMods lo;
	KeyMods hi;
};

static constexpr std::size_t maxOptions = 8;
using KeyAction = KeyCombo<Action>;
using VRange = std::variant<KeyRange, AxisRange>;
template <typename T>
using Options = ktl::fixed_vector<T, maxOptions>;

struct Trigger {
	Options<KeyAction> combos;

	Trigger() = default;
	Trigger(Key key, Action action = Action::ePressed, Mod mod = {}) noexcept;
	Trigger(Key key, Action action, Mods mods) noexcept;

	bool operator()(State const& state) const noexcept;
};

struct Range {
	Options<VRange> matches;

	Range() = default;
	Range(AxisRange axis) noexcept;
	Range(KeyRange key) noexcept;

	f32 operator()(State const& state) const noexcept;
};
} // namespace le::input
