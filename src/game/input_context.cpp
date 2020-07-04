#include <core/log.hpp>
#include <core/utils.hpp>
#include <engine/game/input_context.hpp>
#include <engine/window/window.hpp>

namespace le::input
{
namespace
{
std::unordered_map<std::string_view, Mode> const g_modeMap = {
	{"passthrough", Mode::ePassthrough}, {"block_all", Mode::eBlockAll}, {"block", Mode::eBlockAll}, {"block_on_callback", Mode::eBlockOnCallback}};
}

void Map::addTrigger(std::string const& id, Key key, Action action, Mods::VALUE mods)
{
	auto& binding = bindings[id];
	binding.triggers.push_back({key, action, mods});
}

void Map::addTrigger(std::string const& id, std::initializer_list<Trigger> triggers)
{
	for (auto const& trigger : triggers)
	{
		addTrigger(id, std::get<Key>(trigger), std::get<Action>(trigger), std::get<Mods::VALUE>(trigger));
	}
}

void Map::addState(std::string const& id, Key key)
{
	auto& binding = bindings[id];
	binding.states.push_back({key});
}

void Map::addState(std::string const& id, State state)
{
	auto& binding = bindings[id];
	binding.states.push_back(std::move(state));
}

void Map::addRange(std::string const& id, Axis axis, bool bReverse)
{
	auto& binding = bindings[id];
	Range range = std::make_pair(axis, bReverse);
	binding.ranges.push_back(std::move(range));
}

void Map::addRange(std::string const& id, Key min, Key max)
{
	auto& binding = bindings[id];
	Range range = std::make_pair(min, max);
	binding.ranges.emplace_back(std::move(range));
}

u16 Map::deserialise(GData const& json)
{
	u16 ret = 0;
	auto const bindingsData = json.getDataArray("bindings");
	using bind = std::function<void(std::string const&, GData const&)>;
	auto parse = [&bindingsData, &ret](bind addTrigger, bind addState, bind addRange) {
		for (auto const& entry : bindingsData)
		{
			if (entry.contains("id"))
			{
				auto const id = entry.getString("id");
				auto const triggers = entry.getDataArray("triggers");
				for (auto const& trigger : triggers)
				{
					addTrigger(id, trigger);
					++ret;
				}
				auto const states = entry.getDataArray("states");
				for (auto const& state : states)
				{
					addState(id, state);
					++ret;
				}
				auto const ranges = entry.getDataArray("ranges");
				for (auto const& range : ranges)
				{
					addRange(id, range);
					++ret;
				}
			}
		}
	};
	parse(
		[this](std::string const& id, GData const& trigger) {
			Key const key = parseKey(trigger.getString("key"));
			Action const action = parseAction(trigger.getString("action"));
			Mods::VALUE const mods = parseMods(trigger.getArray("mods"));
			ASSERT(key != Key::eUnknown, "Unknown Key!");
			addTrigger(id, key, action, mods);
		},
		[this](std::string const& id, GData const& state) {
			std::vector<std::string> keysStr;
			if (state.contains("keys"))
			{
				keysStr = state.getArray("keys");
			}
			else if (state.contains("key"))
			{
				keysStr = {state.getString("key")};
			}
			std::vector<Key> keys;
			keys.reserve(keysStr.size());
			for (auto const& keyStr : keysStr)
			{
				auto const key = parseKey(keyStr);
				ASSERT(key != Key::eUnknown, "Unknown Key!");
				keys.push_back(key);
			}
			addState(id, std::move(keys));
		},
		[this](std::string const& id, GData const& range) {
			Key const keyMin = parseKey(range.getString("key_min"));
			Key const keyMax = parseKey(range.getString("key_max"));
			Axis const axis = parseAxis(range.getString("pad_axis"));
			if (keyMin != Key::eUnknown && keyMax != Key::eUnknown)
			{
				addRange(id, keyMin, keyMax);
			}
			else if (axis != Axis::eUnknown)
			{
				bool const bReverse = range.getBool("reverse");
				addRange(id, axis, bReverse);
			}
			else
			{
				ASSERT(false, "Unknown Key/Axis!");
			}
		});
#if defined(LEVK_DEBUG)
	std::string const tName = utils::tName<Map>();
	for (auto const& [id, binding] : bindings)
	{
		LOG_D("[{}] {} bound to {} triggers, {} states, {} ranges", tName, id, binding.triggers.size(), binding.states.size(), binding.ranges.size());
	}
#endif
	return ret;
}

void Map::clear()
{
	bindings.clear();
}

u16 Map::size() const
{
	return (u16)bindings.size();
}

bool Map::isEmpty() const
{
	return bindings.empty();
}

Context::Context(Mode mode, s32 padID) : m_mode(mode), m_padID(padID) {}
Context::Context(Context&&) = default;
Context& Context::operator=(Context&&) = default;
Context::Context(Context const&) = default;
Context& Context::operator=(Context const&) = default;
Context::~Context() = default;

void Context::mapTrigger(std::string const& id, OnTrigger callback)
{
	auto& callbacks = m_callbacks[id];
	callbacks.onTrigger = std::move(callback);
}

void Context::mapState(std::string const& id, OnState callback)
{
	auto& callbacks = m_callbacks[id];
	callbacks.onState = std::move(callback);
}

void Context::mapRange(std::string const& id, OnRange callback)
{
	auto& callbacks = m_callbacks[id];
	callbacks.onRange = std::move(callback);
}

void Context::addTrigger(std::string const& id, Key key, Action action, Mods::VALUE mods)
{
	m_map.addTrigger(id, key, action, mods);
}

void Context::addState(std::string const& id, Key key)
{
	m_map.addState(id, key);
}

void Context::addState(std::string const& id, State state)
{
	m_map.addState(id, std::move(state));
}

void Context::addRange(std::string const& id, Axis axis, bool bReverse)
{
	m_map.addRange(id, axis, bReverse);
}

void Context::addRange(std::string const& id, Key min, Key max)
{
	m_map.addRange(id, min, max);
}

void Context::setMode(Mode mode)
{
	m_mode = mode;
}

void Context::setGamepadID(s32 id)
{
	m_padID = id;
}

u16 Context::deserialise(GData const& json)
{
	if (json.contains("mode"))
	{
		if (auto search = g_modeMap.find(json.getString("mode")); search != g_modeMap.end())
		{
			m_mode = search->second;
		}
	}
	if (json.contains("gamepad_id"))
	{
		m_padID = json.getS32("gamepad_id");
	}
	return m_map.deserialise(json);
}

void Context::import(Map map)
{
	m_map = std::move(map);
}

bool Context::wasFired() const
{
	return m_bFired;
}

bool Context::isConsumed(Snapshot const& snapshot) const
{
	m_bFired = true;
	bool bConsumed = false;
	auto const padID = m_padID;
	auto padSearch = std::find_if(snapshot.padStates.begin(), snapshot.padStates.end(), [padID](auto const& padState) -> bool { return padState.id == padID; });
	Gamepad const* pGamepad = padSearch != snapshot.padStates.end() ? &(*padSearch) : nullptr;
	for (auto const& [key, map] : m_map.bindings)
	{
		if (auto search = m_callbacks.find(key); search != m_callbacks.end())
		{
			auto const& callbacks = search->second;
			if (callbacks.onTrigger)
			{
				// Triggers are only fired once until key is released; so if key is in current snapshot or is pressed but was not held before
				for (auto const& trigger : map.triggers)
				{
					auto const tKey = std::get<Key>(trigger);
					if (isGamepadButton(tKey))
					{
						if (auto search = m_padHeld.find(tKey); search == m_padHeld.end() && pGamepad && pGamepad->isPressed(tKey))
						{
							// All pressed gamepad keys will be added to m_padHeld before returning
							callbacks.onTrigger();
							bConsumed = true;
							break;
						}
					}
					else
					{
						// Keyboard keys will be present in current snapshot
						if (auto search = std::find(snapshot.keys.begin(), snapshot.keys.end(), trigger); search != snapshot.keys.end())
						{
							callbacks.onTrigger();
							bConsumed = true;
							break;
						}
					}
				}
			}
			else if (callbacks.onState)
			{
				// States are fired all the time, and will be active (true) if _any_ registered key is pressed/held
				bool bHeld = false;
				for (auto const& state : map.states)
				{
					bool bCombo = !state.empty();
					for (auto key : state)
					{
						if (pGamepad && isGamepadButton(key))
						{
							bCombo &= pGamepad->isPressed(key);
						}
						else
						{
							bCombo &= std::find(snapshot.held.begin(), snapshot.held.end(), key) != snapshot.held.end();
						}
					}
					bHeld |= bCombo;
				}
				callbacks.onState(bHeld);
				bConsumed = true;
			}
			else if (callbacks.onRange && (!snapshot.padStates.empty() || !snapshot.held.empty()))
			{
				// Ranges are always fired, values are accumulated among all registered axes/key combos and clamped
				f32 value = 0.0f;
				for (auto const& range : map.ranges)
				{
					if (auto pPad = std::get_if<std::pair<Axis, bool>>(&range))
					{
						auto const axis = std::get<Axis>(*pPad);
						f32 const coeff = std::get<bool>(*pPad) ? -1.0f : 1.0f;
						if (isMouseAxis(axis))
						{
							switch (axis)
							{
							default:
								break;
							case Axis::eMouseScrollX:
							{
								value += snapshot.mouseScroll.x;
								break;
							}
							case Axis::eMouseScrollY:
							{
								value += snapshot.mouseScroll.y;
								break;
							}
							}
						}
						else if (pGamepad)
						{
							switch (axis)
							{
							case Axis::eLeftTrigger:
							case Axis::eRightTrigger:
							{
								value += Window::triggerToAxis(pGamepad->axis(axis));
								break;
							}
							default:
							{
								value += pGamepad->axis(axis);
							}
							}
						}
						value *= coeff;
					}
					else if (auto pKey = std::get_if<std::pair<Key, Key>>(&range))
					{
						bool bMin = false;
						bool bMax = false;
						if (pGamepad)
						{
							bMin = isGamepadButton(pKey->first) && pGamepad->isPressed(pKey->first);
							bMax = isGamepadButton(pKey->second) && pGamepad->isPressed(pKey->second);
						}
						bMin |= std::find(snapshot.held.begin(), snapshot.held.end(), pKey->first) != snapshot.held.end();
						bMax |= std::find(snapshot.held.begin(), snapshot.held.end(), pKey->second) != snapshot.held.end();
						f32 const lhs = bMin ? -1.0f : 0.0f;
						f32 const rhs = bMax ? 1.0f : 0.0f;
						value += lhs + rhs;
					}
					else
					{
						ASSERT(false, "Invariant violated!");
					}
				}
				callbacks.onRange(std::clamp(value, -1.0f, 1.0f));
				bConsumed = true;
			}
		}
	}
	if (pGamepad)
	{
		// Store currently held keys; needed to ignore triggers fired above until released
		for (s32 i = (s32)Key::eGamepadButtonBegin; i < (s32)Key::eGamepadButtonEnd; ++i)
		{
			Key key = (Key)i;
			if (pGamepad->isPressed(key))
			{
				m_padHeld.insert(key);
			}
			else
			{
				m_padHeld.erase(key);
			}
		}
	}
	else
	{
		// No gamepad any more, clear all held keys
		m_padHeld.clear();
	}
	bool bRet;
	switch (m_mode)
	{
	default:
	{
		bRet = false;
		break;
	}
	case Mode::eBlockAll:
	{
		bRet = true;
		break;
	}
	case Mode::eBlockOnCallback:
	{
		// True if triggered / has state/range callbacks
		bRet = bConsumed;
	}
	}
	return bRet;
}
} // namespace le::input
