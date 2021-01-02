#include <core/log.hpp>
#include <core/utils.hpp>
#include <engine/game/input_context.hpp>
#include <engine/window/window.hpp>

namespace le::input {
namespace {
std::unordered_map<std::string_view, Mode> const g_modeMap = {
	{"passthrough", Mode::ePassthrough}, {"block_all", Mode::eBlockAll}, {"block", Mode::eBlockAll}, {"block_on_callback", Mode::eBlockOnCallback}};
}

void Map::addTrigger(Hash id, Key key, Action action, Mods::VALUE mods) {
	auto& binding = bindings[id];
	binding.triggers.push_back({key, action, mods});
}

void Map::addTrigger(Hash id, std::initializer_list<Trigger> triggers) {
	for (auto const& trigger : triggers) {
		addTrigger(id, std::get<Key>(trigger), std::get<Action>(trigger), std::get<Mods::VALUE>(trigger));
	}
}

void Map::addState(Hash id, Key key) {
	auto& binding = bindings[id];
	binding.states.push_back({key});
}

void Map::addState(Hash id, State state) {
	auto& binding = bindings[id];
	binding.states.push_back(std::move(state));
}

void Map::addRange(Hash id, Axis axis, bool bReverse) {
	auto& binding = bindings[id];
	Range range = std::make_pair(axis, bReverse);
	binding.ranges.push_back(std::move(range));
}

void Map::addRange(Hash id, Key min, Key max) {
	auto& binding = bindings[id];
	Range range = std::make_pair(min, max);
	binding.ranges.emplace_back(std::move(range));
}

u16 Map::deserialise(dj::node_t const& json) {
	u16 ret = 0;
	auto const pBindings = json.find("bindings");
	if (pBindings) {
		auto parse = [&](auto addTrigger, auto addState, auto addRange) {
			for (auto const& entry : pBindings->as<dj::array_nodes_t>()) {
				if (auto pID = entry->find("id")) {
					auto const id = pID->as<std::string>();
					if (auto pTriggers = entry->find("triggers")) {
						for (auto const& trigger : pTriggers->as<dj::array_nodes_t>()) {
							addTrigger(id, *trigger);
							++ret;
						}
					}
					if (auto pStates = entry->find("states")) {
						for (auto const& state : pStates->as<dj::array_nodes_t>()) {
							addState(id, *state);
							++ret;
						}
					}
					if (auto pRanges = entry->find("ranges")) {
						for (auto const& range : pRanges->as<dj::array_nodes_t>()) {
							addRange(id, *range);
							++ret;
						}
					}
				}
			}
		};
		parse(
			[this](Hash id, dj::node_t const& trigger) {
				Key const key = parseKey(trigger.get("key").as<std::string>());
				Action const action = parseAction(trigger.safe_get("action").as<std::string>());
				Mods::VALUE mods = Mods::eNONE;
				if (auto pMods = trigger.find("mods")) {
					std::vector<std::string> modsVec;
					for (auto const& mod : pMods->as<dj::array_fields_t<std::string>>()) {
						modsVec.push_back(mod);
					}
					mods = parseMods(modsVec);
				}
				ENSURE(key != Key::eUnknown, "Unknown Key!");
				addTrigger(id, key, action, mods);
			},
			[this](Hash id, dj::node_t const& state) {
				std::vector<Key> keys;
				std::vector<std::string> keysStr;
				if (auto pKeys = state.find("keys")) {
					for (auto const& str : pKeys->as<dj::array_fields_t<std::string>>()) {
						auto const key = parseKey(str);
						ENSURE(key != Key::eUnknown, "Unknown Key!");
						keys.push_back(key);
					}
				} else if (auto pKey = state.find("key")) {
					auto const key = parseKey(pKey->as<std::string>());
					ENSURE(key != Key::eUnknown, "Unknown Key!");
					keys.push_back(key);
				}
				addState(id, std::move(keys));
			},
			[this](Hash id, dj::node_t const& range) {
				if (range.contains("key_min") && range.contains("key_max")) {
					Key const keyMin = parseKey(range.get("key_min").as<std::string>());
					Key const keyMax = parseKey(range.get("key_max").as<std::string>());
					addRange(id, keyMin, keyMax);
				} else if (range.contains("pad_axis")) {
					Axis const axis = parseAxis(range.get("pad_axis").as<std::string>());
					bool bReverse = false;
					if (auto pRev = range.find("reverse")) {
						bReverse = pRev->as<bool>();
					}
					addRange(id, axis, bReverse);
				} else {
					ENSURE(false, "Unknown Key/Axis!");
				}
			});
	}
	return ret;
}

void Map::clear() {
	bindings.clear();
}

u16 Map::size() const {
	return (u16)bindings.size();
}

bool Map::empty() const {
	return bindings.empty();
}

Context::Context(Mode mode, s32 padID) : m_mode(mode), m_padID(padID) {
}

void Context::mapTrigger(Hash id, OnTrigger callback) {
	auto& callbacks = m_callbacks[id];
	callbacks.onTrigger = std::move(callback);
}

void Context::mapState(Hash id, OnState callback) {
	auto& callbacks = m_callbacks[id];
	callbacks.onState = std::move(callback);
}

void Context::mapRange(Hash id, OnRange callback) {
	auto& callbacks = m_callbacks[id];
	callbacks.onRange = std::move(callback);
}

void Context::addTrigger(Hash id, Key key, Action action, Mods::VALUE mods) {
	m_map.addTrigger(id, key, action, mods);
}

void Context::addState(Hash id, Key key) {
	m_map.addState(id, key);
}

void Context::addState(Hash id, State state) {
	m_map.addState(id, std::move(state));
}

void Context::addRange(Hash id, Axis axis, bool bReverse) {
	m_map.addRange(id, axis, bReverse);
}

void Context::addRange(Hash id, Key min, Key max) {
	m_map.addRange(id, min, max);
}

void Context::setMode(Mode mode) {
	m_mode = mode;
}

void Context::setGamepadID(s32 id) {
	m_padID = id;
}

u16 Context::deserialise(dj::node_t const& json) {
	if (auto pMode = json.find("mode")) {
		if (auto search = g_modeMap.find(pMode->as<std::string>()); search != g_modeMap.end()) {
			m_mode = search->second;
		}
	}
	if (auto pPadID = json.find("gamepad_id")) {
		m_padID = pPadID->as<s32>();
	}
	return m_map.deserialise(json);
}

void Context::import(Map map) {
	m_map = std::move(map);
}

bool Context::wasFired() const {
	return m_bFired;
}

bool Context::consumed(Snapshot const& snapshot) const {
	m_bFired = true;
	bool bConsumed = false;
	auto const padID = m_padID;
	auto padSearch = std::find_if(snapshot.padStates.begin(), snapshot.padStates.end(), [padID](auto const& padState) -> bool { return padState.id == padID; });
	Gamepad const* pGamepad = padSearch != snapshot.padStates.end() ? &(*padSearch) : nullptr;
	for (auto const& [key, map] : m_map.bindings) {
		if (auto search = m_callbacks.find(key); search != m_callbacks.end()) {
			auto const& callbacks = search->second;
			if (callbacks.onTrigger) {
				// Triggers are only fired once until key is released; so if key is in current snapshot or is pressed but was not held before
				for (auto const& trigger : map.triggers) {
					auto const tKey = std::get<Key>(trigger);
					if (isGamepadButton(tKey)) {
						if (auto search = m_padHeld.find(tKey); search == m_padHeld.end() && pGamepad && pGamepad->pressed(tKey)) {
							// All pressed gamepad keys will be added to m_padHeld before returning
							callbacks.onTrigger();
							bConsumed = true;
							break;
						}
					} else {
						// Keyboard keys will be present in current snapshot
						if (auto search = std::find(snapshot.keys.begin(), snapshot.keys.end(), trigger); search != snapshot.keys.end()) {
							callbacks.onTrigger();
							bConsumed = true;
							break;
						}
					}
				}
			} else if (callbacks.onState) {
				// States are fired all the time, and will be active (true) if _any_ registered key is pressed/held
				bool bHeld = false;
				for (auto const& state : map.states) {
					bool bCombo = !state.empty();
					for (auto key : state) {
						if (pGamepad && isGamepadButton(key)) {
							bCombo &= pGamepad->pressed(key);
						} else {
							bCombo &= std::find(snapshot.held.begin(), snapshot.held.end(), key) != snapshot.held.end();
						}
					}
					bHeld |= bCombo;
				}
				callbacks.onState(bHeld);
				bConsumed = true;
			} else if (callbacks.onRange && (!snapshot.padStates.empty() || !snapshot.held.empty())) {
				// Ranges are always fired, values are accumulated among all registered axes/key combos and clamped
				f32 value = 0.0f;
				for (auto const& range : map.ranges) {
					if (auto pPad = std::get_if<std::pair<Axis, bool>>(&range)) {
						auto const axis = std::get<Axis>(*pPad);
						f32 const coeff = std::get<bool>(*pPad) ? -1.0f : 1.0f;
						if (isMouseAxis(axis)) {
							switch (axis) {
							default:
								break;
							case Axis::eMouseScrollX: {
								value += snapshot.mouseScroll.x;
								break;
							}
							case Axis::eMouseScrollY: {
								value += snapshot.mouseScroll.y;
								break;
							}
							}
						} else if (pGamepad) {
							switch (axis) {
							case Axis::eLeftTrigger:
							case Axis::eRightTrigger: {
								value += triggerToAxis(pGamepad->axis(axis));
								break;
							}
							default: {
								value += pGamepad->axis(axis);
							}
							}
						}
						value *= coeff;
					} else if (auto pKey = std::get_if<std::pair<Key, Key>>(&range)) {
						bool bMin = false;
						bool bMax = false;
						if (pGamepad) {
							bMin = isGamepadButton(pKey->first) && pGamepad->pressed(pKey->first);
							bMax = isGamepadButton(pKey->second) && pGamepad->pressed(pKey->second);
						}
						bMin |= std::find(snapshot.held.begin(), snapshot.held.end(), pKey->first) != snapshot.held.end();
						bMax |= std::find(snapshot.held.begin(), snapshot.held.end(), pKey->second) != snapshot.held.end();
						f32 const lhs = bMin ? -1.0f : 0.0f;
						f32 const rhs = bMax ? 1.0f : 0.0f;
						value += lhs + rhs;
					} else {
						ENSURE(false, "Invariant violated!");
					}
				}
				callbacks.onRange(std::clamp(value, -1.0f, 1.0f));
				bConsumed = true;
			}
		}
	}
	if (pGamepad) {
		// Store currently held keys; needed to ignore triggers fired above until released
		for (s32 i = (s32)Key::eGamepadButtonBegin; i < (s32)Key::eGamepadButtonEnd; ++i) {
			Key key = (Key)i;
			if (pGamepad->pressed(key)) {
				m_padHeld.insert(key);
			} else {
				m_padHeld.erase(key);
			}
		}
	} else {
		// No gamepad any more, clear all held keys
		m_padHeld.clear();
	}
	bool bRet;
	switch (m_mode) {
	default: {
		bRet = false;
		break;
	}
	case Mode::eBlockAll: {
		bRet = true;
		break;
	}
	case Mode::eBlockOnCallback: {
		// True if triggered / has state/range callbacks
		bRet = bConsumed;
	}
	}
	return bRet;
}
} // namespace le::input
