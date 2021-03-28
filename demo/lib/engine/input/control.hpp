#pragma once
#include <variant>
#include <engine/input/input.hpp>

namespace le {
class Control {
  public:
	using Action = Input::Action;
	using Mod = Input::Mod;
	using Mods = Input::Mods;
	using Axis = Input::Axis;

	struct AxisRange {
		std::size_t padID = 0;
		Axis axis = {};
		bool invert = false;
	};
	struct KeyRange {
		Input::KeyMods lo;
		Input::KeyMods hi;
	};

	static constexpr std::size_t maxOptions = 8;
	using KeyAction = Input::KeyCombo<Action>;
	using VRange = std::variant<KeyRange, AxisRange>;
	template <typename T>
	using Options = kt::fixed_vector<T, maxOptions>;

	struct Trigger {
		Options<KeyAction> combos;

		Trigger() = default;
		Trigger(Input::Key key, Action action = Action::ePressed, Mod mod = {}) noexcept;

		bool operator()(Input::State const& state) const noexcept;
	};

	struct Range {
		Options<VRange> matches;

		Range() = default;
		Range(AxisRange axis) noexcept;
		Range(KeyRange key) noexcept;

		f32 operator()(Input::State const& state) const noexcept;
	};
};
} // namespace le
