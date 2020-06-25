#pragma once
#include <deque>
#include <string>
#include <tuple>
#include <variant>
#include <unordered_map>
#include <unordered_set>
#include <glm/vec2.hpp>
#include <engine/window/input_types.hpp>

namespace le
{
class InputMapper
{
private:
	struct RawInput final
	{
		Key key = Key::eUnknown;
		Action action = Action::ePress;
		Mods::VALUE mods = Mods::eNONE;
	};

	struct
	{
		OnInput::Token onInput;
		OnMouse::Token onMouse;
		OnText::Token onText;
	} m_tokens;

protected:
	struct
	{
		std::deque<RawInput> input;
		std::deque<GamepadState> gamepads;
		std::deque<char> text;
		std::unordered_set<Key> held;
		glm::vec2 mousePos;
	} m_raw;

	struct
	{
		std::unordered_map<std::string, std::vector<std::tuple<Key, Action, Mods::VALUE>>> triggers;
		std::unordered_map<std::string, std::vector<Key>> states;
		std::unordered_map<std::string, std::vector<std::variant<std::tuple<PadAxis, bool, s32>, std::pair<Key, Key>>>> ranges;
	} m_map;

public:
	InputMapper();
	InputMapper(InputMapper&&);
	InputMapper& operator=(InputMapper&&);
	virtual ~InputMapper();

public:
	void mapTrigger(std::string id, Key key, Action action = Action::eRelease, Mods::VALUE mods = Mods::eNONE);
	void mapState(std::string id, Key key);
	void mapRange(std::string id, PadAxis axis, bool bReverse = false, s32 gamepadID = 0);
	void mapRange(std::string id, Key up, Key down);

	bool isTriggered(std::string const& triggerID) const;
	bool isHeld(std::string const& stateID) const;
	f32 range(std::string const& rangeID) const;

public:
	void flush();
	void clear();
};
} // namespace le
