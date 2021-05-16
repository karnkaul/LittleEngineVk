#include <engine/input/control.hpp>
#include <engine/input/state.hpp>
#include <window/desktop_instance.hpp>

namespace le::input {
Trigger::Trigger(Key key, Action action, Mod mod) noexcept {
	KeyAction ka;
	ka.key = key;
	ka.t = action;
	ka.mods.add(mod);
	combos.push_back(ka);
}

bool Trigger::operator()(State const& state) const noexcept {
	for (auto const& combo : combos) {
		if (combo.key != Key::eUnknown) {
			switch (combo.t) {
			case Action::ePressed: {
				if (auto k = state.pressed(combo.key)) { return k->mods.all(combo.mods.bits); }
				break;
			}
			case Action::eReleased: {
				if (auto k = state.released(combo.key)) { return k->mods.all(combo.mods.bits); }
				break;
			}
			case Action::eHeld: {
				if (auto k = state.held(combo.key)) { return k->mods.all(combo.mods.bits); }
				break;
			}
			default: break;
			}
		}
	}
	return false;
}

Range::Range(AxisRange axis) noexcept { matches.push_back(axis); }

Range::Range(KeyRange key) noexcept { matches.push_back(key); }

f32 Range::operator()(State const& state) const noexcept {
	f32 ret = 0.0f;
	for (auto const& match : matches) {
		if (auto const& ax = std::get_if<AxisRange>(&match)) {
			Gamepad const* pad = ax->padID < state.gamepads.size() ? &state.gamepads[ax->padID] : nullptr;
			switch (ax->axis) {
			case Axis::eMouseScrollX: {
				ret += state.cursor.scroll.x;
				break;
			}
			case Axis::eMouseScrollY: {
				ret += state.cursor.scroll.y;
				break;
			}
			case Axis::eLeftTrigger:
			case Axis::eRightTrigger: {
				if (pad) { ret += window::triggerToAxis(pad->axis(ax->axis)); }
				break;
			}
			default: {
				if (pad) { ret += pad->axis(ax->axis); }
				break;
			}
			}
			return ax->invert ? -ret : ret;
		} else if (auto const& ky = std::get_if<KeyRange>(&match)) {
			auto const lo = state.held(ky->lo.key);
			auto const hi = state.held(ky->hi.key);
			if (lo || hi) {
				f32 const l = lo && lo->mods.all(ky->lo.mods) ? -1.0f : 0.0f;
				f32 const r = hi && hi->mods.all(ky->hi.mods) ? 1.0f : 0.0f;
				ret += (l + r);
			}
		}
	}
	return ret;
}
} // namespace le::input
