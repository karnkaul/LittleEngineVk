#include <variant>
#include <engine/input/input.hpp>

namespace le {
class Control {
  public:
	template <typename T>
	using Combo = Input::KeyCombo<T>;
	using Action = Input::Action;
	using Mod = Input::Mod;
	using Mods = Input::Mods;
	using Axis = Input::Axis;

	static constexpr std::size_t maxOptions = 8;

	template <typename T>
	using Options = kt::fixed_vector<T, maxOptions>;

	struct Trigger {
		Options<Combo<Action>> combos;

		Trigger() = default;
		Trigger(Input::Key key, Action action = Action::ePressed, Mod mod = {}) noexcept;

		bool operator()(Input::State const& state) const noexcept;
	};

	struct State {
		Options<Input::KeyMods> matches;

		bool operator()(Input::State const& state) const noexcept;
	};

	struct Range {
		struct Ax {
			std::size_t padID = 0;
			Axis axis{};
			bool reverse{};
		};
		struct Ky {
			Input::KeyMods lo{};
			Input::KeyMods hi{};
		};

		Options<std::variant<Ax, Ky>> matches;

		Range() = default;
		Range(Ax axis) noexcept;
		Range(Ky key) noexcept;

		f32 operator()(Input::State const& state) const noexcept;
	};
};
} // namespace le
