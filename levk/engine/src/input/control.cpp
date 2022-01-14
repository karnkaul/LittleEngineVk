#include <levk/engine/input/control.hpp>
#include <levk/engine/input/state.hpp>

namespace le::input {
bool Trigger::operator()(State const& state) const noexcept {
	for (auto const& combo : combos) {
		if (auto const ev = combo.get_if<KeyEvent>()) {
			if (auto const mods = state.mods(ev->key, ev->action)) { return *mods == ev->mods; }
		} else if (auto const hold = combo.get_if<Hold>()) {
			return state.held(hold->key);
		}
	}
	return false;
}

f32 Range::operator()(State const& state) const noexcept {
	f32 ret = 0.0f;
	for (auto const& match : matches) {
		if (auto const ax = match.get_if<AxisRange>()) {
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
		} else if (auto const key = match.get_if<KeyRange>()) {
			f32 const l = key->lo > Key::eUnknown && state.held(key->lo) ? -1.0f : 0.0f;
			f32 const r = key->hi > Key::eUnknown && state.held(key->hi) ? 1.0f : 0.0f;
			return l + r;
		}
	}
	return ret;
}
} // namespace le::input
