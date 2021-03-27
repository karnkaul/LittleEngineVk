#include <engine/input/control.hpp>
#include <window/desktop_instance.hpp>

namespace le {
Control::Trigger::Trigger(Input::Key key, Action action, Mod mod) noexcept {
	Combo<Action> combo;
	combo.key = key;
	combo.t = action;
	combo.mods.add(mod);
	combos.push_back(combo);
}

bool Control::Trigger::operator()(Input::State const& state) const noexcept {
	for (auto const& combo : combos) {
		if (combo.key != Input::Key::eUnknown) {
			switch (combo.t) {
			case Action::ePressed: {
				if (auto k = state.pressed(combo.key)) {
					return k->mods.all(combo.mods.bits);
				}
				break;
			}
			case Action::eReleased: {
				if (auto k = state.released(combo.key)) {
					return k->mods.all(combo.mods.bits);
				}
				break;
			}
			case Action::eHeld: {
				if (auto k = state.held(combo.key)) {
					return k->mods.all(combo.mods.bits);
				}
				break;
			}
			default:
				break;
			}
		}
	}
	return false;
}

bool Control::State::operator()(Input::State const& state) const noexcept {
	for (auto const& match : matches) {
		if (match.key != Input::Key::eUnknown) {
			if (auto key = state.acted(match.key)) {
				return key->mods.all(match.mods);
			}
		}
	}
	return false;
}

Control::Range::Range(Ax axis) noexcept {
	matches.push_back(axis);
}

Control::Range::Range(Ky key) noexcept {
	matches.push_back(key);
}

f32 Control::Range::operator()(Input::State const& state) const noexcept {
	f32 ret = 0.0f;
	for (auto const& match : matches) {
		if (auto const& ax = std::get_if<Ax>(&match); ax && ax->padID < state.gamepads.size()) {
			Input::Gamepad const& pad = state.gamepads[ax->padID];
			switch (ax->axis) {
			case Axis::eLeftTrigger:
			case Axis::eRightTrigger: {
				ret += window::triggerToAxis(pad.axis(ax->axis));
				break;
			}
			default: {
				ret += pad.axis(ax->axis);
				break;
			}
			}
			return ax->reverse ? -ret : ret;
		} else if (auto const& ky = std::get_if<Ky>(&match)) {
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
} // namespace le
