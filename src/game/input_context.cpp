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

void Map::addState(std::string const& id, State state)
{
	auto& binding = bindings[id];
	binding.states.push_back(state);
}

void Map::addState(std::string const& id, std::initializer_list<State> states)
{
	for (auto const& state : states)
	{
		addState(id, state);
	}
}

void Map::addRange(std::string const& id, PadAxis axis, bool bReverse, s32 padID)
{
	auto& binding = bindings[id];
	Range range = std::make_tuple(axis, bReverse, padID);
	binding.ranges.push_back(std::move(range));
}

void Map::addRange(std::string const& id, Key min, Key max)
{
	auto& binding = bindings[id];
	Range range = std::make_pair(min, max);
	binding.ranges.emplace_back(std::move(range));
}

void Map::addRange(std::string const& id, std::initializer_list<Range> ranges)
{
	for (auto const& range : ranges)
	{
		auto& binding = bindings[id];
		binding.ranges.push_back(std::move(range));
	}
}

u16 Map::deserialise(GData const& json)
{
	u16 ret = 0;
	auto const bindingsData = json.getGDatas("bindings");
	using bind = std::function<void(std::string const&, GData const&)>;
	auto parse = [&bindingsData, &ret](bind addTrigger, bind addState, bind addRange) {
		for (auto const& entry : bindingsData)
		{
			if (entry.contains("id"))
			{
				auto const id = entry.getString("id");
				auto const triggers = entry.getGDatas("triggers");
				for (auto const& trigger : triggers)
				{
					addTrigger(id, trigger);
					++ret;
				}
				auto const states = entry.getGDatas("states");
				for (auto const& state : states)
				{
					addState(id, state);
					++ret;
				}
				auto const ranges = entry.getGDatas("ranges");
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
			Mods::VALUE const mods = parseMods(trigger.getVecString("mods"));
			ASSERT(key != Key::eUnknown, "Unknown Key!");
			addTrigger(id, key, action, mods);
		},
		[this](std::string const& id, GData const& state) {
			{
				Key const key = parseKey(state.getString("key"));
				ASSERT(key != Key::eUnknown, "Unknown Key!");
				addState(id, key);
			}
		},
		[this](std::string const& id, GData const& range) {
			Key const keyMin = parseKey(range.getString("key_min"));
			Key const keyMax = parseKey(range.getString("key_max"));
			PadAxis const axis = parseAxis(range.getString("pad_axis"));
			if (keyMin != Key::eUnknown && keyMax != Key::eUnknown)
			{
				addRange(id, keyMin, keyMax);
			}
			else if (axis != PadAxis::eUnknown)
			{
				bool const bReverse = range.getBool("reverse", false);
				s32 const padID = range.getS32("padID", 0);
				addRange(id, axis, bReverse, padID);
			}
			else
			{
				ASSERT(false, "Unknown Key/PadAxis!");
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

Context::Context(Mode mode) : m_mode(mode) {}
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

void Context::addState(std::string const& id, State state)
{
	m_map.addState(id, state);
}

void Context::addRange(std::string const& id, PadAxis axis, bool bReverse, s32 padID)
{
	m_map.addRange(id, axis, bReverse, padID);
}

void Context::addRange(std::string const& id, Key min, Key max)
{
	m_map.addRange(id, min, max);
}

void Context::setMode(Mode mode)
{
	m_mode = mode;
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
	return m_map.deserialise(json);
}

void Context::import(Map map)
{
	m_map = std::move(map);
}

bool Context::isConsumed(Snapshot const& snapshot) const
{
	bool bFired = false;
	for (auto const& [key, map] : m_map.bindings)
	{
		if (auto search = m_callbacks.find(key); search != m_callbacks.end())
		{
			auto const& callbacks = search->second;
			if (callbacks.onTrigger && !snapshot.keys.empty())
			{
				auto search = std::find_if(map.triggers.begin(), map.triggers.end(), [&snapshot](auto const& trigger) -> bool {
					return std::find(snapshot.keys.begin(), snapshot.keys.end(), trigger) != snapshot.keys.end();
				});
				if (search != map.triggers.end())
				{
					callbacks.onTrigger();
					bFired = true;
				}
			}
			else if (callbacks.onState && !snapshot.held.empty())
			{
				auto search = std::find_if(map.states.begin(), map.states.end(), [&snapshot](auto const& key) -> bool {
					return std::find(snapshot.held.begin(), snapshot.held.end(), key) != snapshot.held.end();
				});
				if (search != map.states.end())
				{
					callbacks.onState();
					bFired = true;
				}
			}
			else if (callbacks.onRange && (!snapshot.padStates.empty() || !snapshot.held.empty()))
			{
				f32 value = 0.0f;
				for (auto const& range : map.ranges)
				{
					if (auto pPad = std::get_if<std::tuple<PadAxis, bool, s32>>(&range))
					{
						auto const id = std::get<s32>(*pPad);
						auto search = std::find_if(snapshot.padStates.begin(), snapshot.padStates.end(),
												   [id](auto const& padState) -> bool { return padState.id == id; });
						if (search != snapshot.padStates.end())
						{
							auto const& padState = *search;
							auto const axis = std::get<PadAxis>(*pPad);
							f32 const coeff = std::get<bool>(*pPad) ? -1.0f : 1.0f;
							switch (axis)
							{
							case PadAxis::eLeftTrigger:
							case PadAxis::eRightTrigger:
							{
								value += Window::triggerToAxis(padState.axis(axis));
								break;
							}
							default:
							{
								value += padState.axis(axis);
							}
							}
							value *= coeff;
						}
					}
					else if (auto pKey = std::get_if<std::pair<Key, Key>>(&range))
					{
						bool const key1 = std::find(snapshot.held.begin(), snapshot.held.end(), pKey->first) != snapshot.held.end();
						bool const key2 = std::find(snapshot.held.begin(), snapshot.held.end(), pKey->second) != snapshot.held.end();
						f32 const lhs = key1 ? 1.0f : 0.0f;
						f32 const rhs = key2 ? -1.0f : 0.0f;
						value += lhs + rhs;
					}
					else
					{
						ASSERT(false, "Invariant violated!");
					}
					callbacks.onRange(value);
					bFired = true;
				}
			}
		}
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
		bRet = bFired;
	}
	}
	return bRet;
}
} // namespace le::input
