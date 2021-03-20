#pragma once
#include <tuple>
#include <unordered_set>
#include <core/ref.hpp>
#include <core/token_gen.hpp>
#include <engine/ibase.hpp>
#include <kt/enum_flags/enum_flags.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>
#include <window/event_queue.hpp>

namespace le {
namespace window {
class IInstance;
class DesktopInstance;
} // namespace window
struct Viewport;

class Input {
  public:
	enum class Action { ePressed, eHeld, eReleased, eCOUNT_ };
	using Mask = kt::enum_flags<Action>;

	enum class Focus { eUnchanged, eGained, eLost };
	using Gamepad = window::Gamepad;
	using EventQueue = window::EventQueue;
	using DesktopInstance = window::DesktopInstance;

	struct Cursor {
		glm::vec2 position = {};
		glm::vec2 screenPos = {};
		glm::vec2 scroll = {};
	};

	struct Key {
		window::Key key;
		Mask actions;

		constexpr friend bool operator==(Key l, Key r) noexcept {
			return l.key == r.key && l.actions == r.actions;
		}
	};

	struct State;
	struct Out;

	class IContext;

	Out update(EventQueue queue, Viewport const& view, bool consume = true, DesktopInstance const* pDI = {}) noexcept;

  private:
	using KeySet = std::unordered_set<window::Key>;

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

	kt::fixed_vector<Key, 16> keys;
	Cursor cursor;
	View<Gamepad> gamepads;
	View<window::Event::Cursor> others;
	View<char> text;
	Focus focus = Focus::eUnchanged;
	bool suspended = false;

	Mask action(window::Key key) const noexcept;
	bool pressed(window::Key key) const noexcept;
	bool held(window::Key key) const noexcept;
	bool released(window::Key key) const noexcept;
	bool any(List<window::Key> keys, Mask mask = Mask::inverse()) const noexcept;
	bool all(List<window::Key> keys, Mask mask = Mask::inverse()) const noexcept;
};

struct Input::Out {
	State state;
	window::EventQueue residue;
};

class Input::IContext : public IBase {
  public:
	virtual bool block(State const& state) = 0;

	Token m_inputToken;
};

// impl

inline Input::Mask Input::State::action(window::Key key) const noexcept {
	Mask ret;
	for (Key const& k : keys) {
		if (k.key == key) {
			ret = k.actions;
			break;
		}
	}
	return ret;
}
inline bool Input::State::pressed(window::Key key) const noexcept {
	return action(key).test(Action::ePressed);
}
inline bool Input::State::held(window::Key key) const noexcept {
	return action(key).test(Action::eHeld);
}
inline bool Input::State::released(window::Key key) const noexcept {
	return action(key).test(Action::eReleased);
}
inline bool Input::State::any(List<window::Key> keys, Mask mask) const noexcept {
	for (window::Key const k : keys) {
		if (action(k).any(mask)) {
			return true;
		}
	}
	return false;
}
inline bool Input::State::all(List<window::Key> keys, Mask mask) const noexcept {
	for (window::Key const k : keys) {
		if (!action(k).any(mask)) {
			return false;
		}
	}
	return keys.size() > 0;
}
} // namespace le
