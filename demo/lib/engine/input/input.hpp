#pragma once
#include <tuple>
#include <unordered_set>
#include <core/ref.hpp>
#include <core/token_gen.hpp>
#include <engine/ibase.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>
#include <kt/result/result.hpp>
#include <window/event_queue.hpp>

namespace le {
namespace window {
class IInstance;
class DesktopInstance;
} // namespace window
struct Viewport;

class Input {
  public:
	enum class Action { ePressed = 1 << 0, eHeld = 1 << 1, eReleased = 1 << 2 };
	using ActMask = kt::uint_flags<>;

	enum class Focus { eUnchanged, eGained, eLost };
	using Gamepad = window::Gamepad;
	using EventQueue = window::EventQueue;
	using DesktopInstance = window::DesktopInstance;
	using Mods = window::Mods;

	static constexpr ActMask every = ActMask::combine(Action::ePressed, Action::eHeld, Action::eReleased);

	struct Cursor {
		glm::vec2 position = {};
		glm::vec2 screenPos = {};
		glm::vec2 scroll = {};
	};

	struct Key {
		window::Key key = window::Key::eUnknown;
		window::Mods mods{};
	};

	struct KeyAct : Key {
		ActMask actions;
	};

	struct State;
	struct Out;

	class IReceiver;

	Out update(EventQueue queue, Viewport const& view, bool consume = true, DesktopInstance const* pDI = {}) noexcept;

  private:
	struct KeySet {
		std::array<Key, 8> keys{};

		bool insert(window::Key k, window::Mods const& mods);
		bool erase(window::Key k) noexcept;
	};

	static void copy(KeySet const& in, kt::fixed_vector<KeyAct, 16>& out_keys, Input::Action action);
	bool extract(window::Event const& event, State& out_state) noexcept;

	struct {
		kt::fixed_vector<window::Gamepad, 8> gamepads;
		kt::fixed_vector<window::Event::Cursor, 8> others;
		kt::fixed_vector<char, 4> text;
		KeySet pressed;
		KeySet released;
	} m_transient;

	struct {
		KeySet held;
		glm::vec2 cursor = {};
		bool suspended = false;
	} m_persistent;
};

struct Input::State {
	template <typename T>
	using List = std::initializer_list<T>;
	template <typename T>
	using Res = kt::result<T, void>;

	kt::fixed_vector<KeyAct, 16> keys;
	Cursor cursor;
	View<Gamepad> gamepads;
	View<window::Event::Cursor> others;
	View<char> text;
	Focus focus = Focus::eUnchanged;
	bool suspended = false;

	KeyAct const& keyMask(window::Key key) const noexcept;
	window::Mods mods(window::Key key) const noexcept;
	ActMask actions(window::Key key) const noexcept;

	Res<Key> acted(window::Key) const noexcept;
	Res<Key> acted(window::Key key, Action action) const noexcept;
	Res<Key> pressed(window::Key key) const noexcept;
	Res<Key> held(window::Key key) const noexcept;
	Res<Key> released(window::Key key) const noexcept;

	bool any(List<window::Key> keys, ActMask mask = every) const noexcept;
	bool all(List<window::Key> keys, ActMask mask = every) const noexcept;
};

struct Input::Out {
	State state;
	window::EventQueue residue;
};

class Input::IReceiver : public IBase {
  public:
	virtual bool block(State const& state) = 0;

	Token m_inputToken;
};

// impl

inline Input::KeyAct const& Input::State::keyMask(window::Key key) const noexcept {
	static constexpr KeyAct blank{};
	if (key == blank.key) {
		return blank;
	}
	for (auto const& k : keys) {
		if (k.key == key) {
			return k;
		}
	}
	return blank;
}
inline window::Mods Input::State::mods(window::Key key) const noexcept {
	window::Mods ret{};
	for (KeyAct const& k : keys) {
		if (k.key == key) {
			ret = k.mods;
			break;
		}
	}
	return ret;
}
inline Input::ActMask Input::State::actions(window::Key key) const noexcept {
	ActMask ret{};
	for (KeyAct const& k : keys) {
		if (k.key == key) {
			ret = k.actions;
			break;
		}
	}
	return ret;
}
inline Input::State::Res<Input::Key> Input::State::acted(window::Key key) const noexcept {
	KeyAct const k = keyMask(key);
	if (k.key != window::Key::eUnknown) {
		return k;
	}
	return kt::null_result;
}
inline Input::State::Res<Input::Key> Input::State::acted(window::Key key, Action action) const noexcept {
	KeyAct const k = keyMask(key);
	if (k.key != window::Key::eUnknown && k.actions[action]) {
		return k;
	}
	return kt::null_result;
}
inline Input::State::Res<Input::Key> Input::State::pressed(window::Key key) const noexcept {
	return acted(key, Action::ePressed);
}
inline Input::State::Res<Input::Key> Input::State::held(window::Key key) const noexcept {
	return acted(key, Action::eHeld);
}
inline Input::State::Res<Input::Key> Input::State::released(window::Key key) const noexcept {
	return acted(key, Action::eReleased);
}
inline bool Input::State::any(List<window::Key> keys, ActMask mask) const noexcept {
	for (window::Key const k : keys) {
		if (actions(k).any(mask)) {
			return true;
		}
	}
	return false;
}
inline bool Input::State::all(List<window::Key> keys, ActMask mask) const noexcept {
	for (window::Key const k : keys) {
		if (!actions(k).any(mask)) {
			return false;
		}
	}
	return keys.size() > 0;
}
} // namespace le
