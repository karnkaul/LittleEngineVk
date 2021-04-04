#pragma once
#include <tuple>
#include <unordered_set>
#include <core/maths.hpp>
#include <core/ref.hpp>
#include <core/tagged_store.hpp>
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

using InputTag = Tag<>;

class Input {
  public:
	enum class Action { ePressed = 1 << 0, eHeld = 1 << 1, eReleased = 1 << 2 };
	using ActionMask = kt::uint_flags<>;
	static constexpr ActionMask allActions = ActionMask::fill(maths::enumEnd(Action::eReleased));

	enum class Focus { eUnchanged, eGained, eLost };
	using Gamepad = window::Gamepad;
	using Event = window::Event;
	using EventQueue = window::EventQueue;
	using DesktopInstance = window::DesktopInstance;

	using Key = window::Key;
	using Axis = window::Axis;
	using Mod = window::Mod;
	using Mods = window::Mods;

	struct Cursor {
		glm::vec2 position = {};
		glm::vec2 screenPos = {};
		glm::vec2 scroll = {};
	};

	struct KeyMods {
		Key key = Key::eUnknown;
		Mods mods{};

		constexpr KeyMods() = default;
		constexpr KeyMods(Key k) noexcept;
		constexpr KeyMods(Key k, Mod mod) noexcept;
		constexpr KeyMods(Key k, Mods m) noexcept;
	};

	template <typename T>
	struct KeyCombo : KeyMods {
		T t{};
	};
	using KeyAct = KeyCombo<ActionMask>;

	struct State;
	struct Out;

	class IReceiver;

	Out update(EventQueue queue, Viewport const& view, bool consume = true, DesktopInstance const* pDI = {}) noexcept;

  private:
	struct KeySet {
		std::array<KeyMods, 8> keys{};

		bool insert(Key k, Mods const& mods);
		bool erase(Key k) noexcept;
	};

	static void copy(KeySet const& in, kt::fixed_vector<KeyAct, 16>& out_keys, Input::Action action);
	bool extract(Event const& event, State& out_state) noexcept;

	struct {
		kt::fixed_vector<Gamepad, 8> gamepads;
		kt::fixed_vector<Event::Cursor, 8> others;
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

template <>
struct Input::KeyCombo<void> : KeyMods {};

struct Input::State {
	template <typename T>
	using List = std::initializer_list<T>;
	template <typename T>
	using Res = kt::result<T, void>;

	kt::fixed_vector<KeyAct, 16> keys;
	Cursor cursor;
	View<Gamepad> gamepads;
	View<Event::Cursor> others;
	View<char> text;
	Focus focus = Focus::eUnchanged;
	bool suspended = false;

	KeyAct const& keyMask(Key key) const noexcept;
	Mods mods(Key key) const noexcept;
	ActionMask actions(Key key) const noexcept;

	Res<KeyMods> acted(Key) const noexcept;
	Res<KeyMods> acted(Key key, Action action) const noexcept;
	Res<KeyMods> pressed(Key key) const noexcept;
	Res<KeyMods> held(Key key) const noexcept;
	Res<KeyMods> released(Key key) const noexcept;

	bool any(List<Key> keys, ActionMask mask = allActions) const noexcept;
	bool all(List<Key> keys, ActionMask mask = allActions) const noexcept;
};

struct Input::Out {
	State state;
	EventQueue residue;
};

class Input::IReceiver : public IBase {
  public:
	virtual bool block(State const& state) = 0;

	InputTag m_inputTag;
};

// impl

constexpr Input::KeyMods::KeyMods(Key k) noexcept : key(k) {
}
constexpr Input::KeyMods::KeyMods(Key k, Mod mod) noexcept : key(k) {
	mods.add(mod);
}
constexpr Input::KeyMods::KeyMods(Key k, Mods m) noexcept : key(k), mods(m) {
}
inline Input::KeyAct const& Input::State::keyMask(Key key) const noexcept {
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
inline Input::Mods Input::State::mods(Key key) const noexcept {
	Mods ret{};
	for (KeyAct const& k : keys) {
		if (k.key == key) {
			ret = k.mods;
			break;
		}
	}
	return ret;
}
inline Input::ActionMask Input::State::actions(Key key) const noexcept {
	ActionMask ret{};
	for (KeyAct const& k : keys) {
		if (k.key == key) {
			ret = k.t;
			break;
		}
	}
	return ret;
}
inline Input::State::Res<Input::KeyMods> Input::State::acted(Key key) const noexcept {
	KeyAct const k = keyMask(key);
	if (k.key != Key::eUnknown) {
		return k;
	}
	return kt::null_result;
}
inline Input::State::Res<Input::KeyMods> Input::State::acted(Key key, Action action) const noexcept {
	KeyAct const k = keyMask(key);
	if (k.key != Key::eUnknown && k.t[action]) {
		return k;
	}
	return kt::null_result;
}
inline Input::State::Res<Input::KeyMods> Input::State::pressed(Key key) const noexcept {
	return acted(key, Action::ePressed);
}
inline Input::State::Res<Input::KeyMods> Input::State::held(Key key) const noexcept {
	return acted(key, Action::eHeld);
}
inline Input::State::Res<Input::KeyMods> Input::State::released(Key key) const noexcept {
	return acted(key, Action::eReleased);
}
inline bool Input::State::any(List<Key> keys, ActionMask mask) const noexcept {
	for (Key const k : keys) {
		if (actions(k).any(mask)) {
			return true;
		}
	}
	return false;
}
inline bool Input::State::all(List<Key> keys, ActionMask mask) const noexcept {
	for (Key const k : keys) {
		if (!actions(k).any(mask)) {
			return false;
		}
	}
	return keys.size() > 0;
}
} // namespace le
