#include <algorithm>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <engine/game/input_mapper.hpp>
#include <levk_impl.hpp>

namespace le
{
InputMapper::InputMapper()
{
	auto pWindow = engine::window();
	ASSERT(pWindow, "Window is null!");
	m_tokens.onInput = pWindow->registerInput([this](Key key, Action action, Mods::VALUE mods) {
		m_raw.input.push_back({key, action, mods});
		if (action == Action::ePress)
		{
			m_raw.held.insert(key);
		}
		else if (action == Action::eRelease)
		{
			m_raw.held.erase(key);
		}
	});
	m_tokens.onText = pWindow->registerText([this](char c) { m_raw.text.push_back(c); });
	m_tokens.onMouse = pWindow->registerMouse([this](f64 x, f64 y) { m_raw.mousePos = {(f32)x, (f32)y}; });
}

InputMapper::InputMapper(InputMapper&&) = default;
InputMapper& InputMapper::operator=(InputMapper&&) = default;
InputMapper::~InputMapper() = default;

void InputMapper::mapTrigger(std::string id, Key key, Action action, Mods::VALUE mods)
{
	m_map.triggers[std::move(id)].push_back(std::make_tuple(key, action, mods));
}

void InputMapper::mapState(std::string id, Key key)
{
	m_map.states[std::move(id)].push_back(key);
}

void InputMapper::mapRange(std::string id, PadAxis axis, bool bReverse, s32 gamepadID)
{
	m_map.ranges[std::move(id)].push_back(std::make_tuple(axis, bReverse, gamepadID));
}

void InputMapper::mapRange(std::string id, Key up, Key down)
{
	m_map.ranges[std::move(id)].push_back(std::make_pair(up, down));
}

bool InputMapper::isTriggered(std::string const& triggerID) const
{
	if (auto const search = m_map.triggers.find(triggerID); search != m_map.triggers.end())
	{
		auto const& vec = search->second;
		auto iter = std::find_if(vec.begin(), vec.end(), [this](auto const& tuple) -> bool {
			return std::find_if(m_raw.input.begin(), m_raw.input.end(),
								[&tuple](auto const& input) -> bool {
									auto const key = std::get<Key>(tuple);
									auto const action = std::get<Action>(tuple);
									auto const mods = std::get<Mods::VALUE>(tuple);
									return input.key == key && input.action == action && (mods == Mods::eNONE || mods & input.mods);
								})
				   != m_raw.input.end();
		});
		return iter != vec.end();
	}
	return false;
}

bool InputMapper::isHeld(std::string const& stateID) const
{
	if (auto const search = m_map.states.find(stateID); search != m_map.states.end())
	{
		auto const& vec = search->second;
		auto iter = std::find_if(vec.begin(), vec.end(), [this](Key const key) { return m_raw.held.find(key) != m_raw.held.end(); });
		return iter != vec.end();
	}
	return false;
}

f32 InputMapper::range(std::string const& id) const
{
	f32 ret = 0.0f;
	if (auto const search = m_map.ranges.find(id); search != m_map.ranges.end())
	{
		for (auto const& variant : search->second)
		{
			if (auto pPad = std::get_if<std::tuple<PadAxis, bool, s32>>(&variant))
			{
				auto const& gamepadState = Window::gamepadState(std::get<s32>(*pPad));
				auto const axis = std::get<PadAxis>(*pPad);
				f32 const coeff = std::get<bool>(*pPad) ? -1.0f : 1.0f;
				switch (axis)
				{
				case PadAxis::eLeftTrigger:
				case PadAxis::eRightTrigger:
				{
					ret += (Window::triggerToAxis(gamepadState.axis(axis)) * coeff);
					break;
				}
				default:
				{
					ret += (gamepadState.axis(axis) * coeff);
				}
				}
			}
			else if (auto pKey = std::get_if<std::pair<Key, Key>>(&variant))
			{
				bool const key1 = std::find(m_raw.held.begin(), m_raw.held.end(), pKey->first) != m_raw.held.end();
				bool const key2 = std::find(m_raw.held.begin(), m_raw.held.end(), pKey->second) != m_raw.held.end();
				f32 const lhs = key1 ? 1.0f : 0.0f;
				f32 const rhs = key2 ? -1.0f : 0.0f;
				ret += lhs + rhs;
			}
		}
	}
	return ret;
}

void InputMapper::flush()
{
	m_raw.input.clear();
	m_raw.text.clear();
}

void InputMapper::clear()
{
	flush();
	m_map = {};
}
} // namespace le
