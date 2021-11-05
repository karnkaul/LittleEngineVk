#pragma once
#include <engine/input/types.hpp>
#include <ktl/either.hpp>

namespace le::input {
struct State;

struct AxisRange {
	Axis axis = {};
	std::size_t padID = 0;
	bool invert = false;
};
struct KeyRange {
	Key lo{};
	Key hi{};
};
struct Hold {
	Key key{};
};

using VTrigger = ktl::either<KeyEvent, Hold>;
using VRange = ktl::either<KeyRange, AxisRange>;

static constexpr std::size_t maxOptions = 8;
template <typename T>
using Options = ktl::fixed_vector<T, maxOptions>;

struct Trigger {
	Options<VTrigger> combos;

	Trigger() = default;
	Trigger(Key key, Action action = Action::ePress, Mods mods = {}) noexcept { add(key, action, mods); }
	Trigger(Hold hold) noexcept { add(hold); }

	bool add(Key key, Action action = Action::ePress, Mods mods = {}) noexcept { return add({key, action, mods}); }
	bool add(KeyEvent event) noexcept;
	bool add(Hold hold) noexcept;

	bool operator()(State const& state) const noexcept;
};

struct Range {
	Options<VRange> matches;

	Range() = default;
	Range(AxisRange axis) noexcept { add(axis); }
	Range(KeyRange key) noexcept { add(key); }

	bool add(AxisRange axis) noexcept;
	bool add(KeyRange key) noexcept;

	f32 operator()(State const& state) const noexcept;
};

// impl

inline bool Trigger::add(KeyEvent event) noexcept {
	if (combos.has_space()) {
		combos.push_back(event);
		return true;
	}
	return false;
}

inline bool Trigger::add(Hold hold) noexcept {
	if (combos.has_space()) {
		combos.push_back(hold);
		return true;
	}
	return false;
}

inline bool Range::add(AxisRange axis) noexcept {
	if (matches.has_space()) {
		matches.push_back(axis);
		return true;
	}
	return false;
}

inline bool Range::add(KeyRange key) noexcept {
	if (matches.has_space()) {
		matches.push_back(key);
		return true;
	}
	return false;
}
} // namespace le::input
