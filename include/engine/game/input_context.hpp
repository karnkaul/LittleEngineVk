#pragma once
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <glm/vec2.hpp>
#include <core/gdata.hpp>
#include <engine/window/input_types.hpp>

namespace le::input
{
struct Snapshot final
{
	std::vector<Gamepad> padStates;
	std::vector<std::tuple<Key, Action, Mods::VALUE>> keys;
	std::vector<Key> held;
	std::vector<char> text;
	glm::vec2 mouseScroll = glm::vec2(0.0f);
};

using Trigger = std::tuple<Key, Action, Mods::VALUE>;
using State = std::vector<Key>;
using Range = std::variant<std::pair<Axis, bool>, std::pair<Key, Key>>;

using OnTrigger = std::function<void()>;
using OnState = std::function<void(bool)>;
using OnRange = std::function<void(f32)>;

enum class Mode : s8
{
	ePassthrough,
	eBlockAll,
	eBlockOnCallback,
	eCOUNT_
};

struct Binding final
{
	std::vector<Trigger> triggers;
	std::vector<State> states;
	std::vector<Range> ranges;
};

struct Callback final
{
	OnTrigger onTrigger;
	OnState onState;
	OnRange onRange;
};

struct Map final
{
	std::unordered_map<std::string, Binding> bindings;

	void addTrigger(std::string const& id, Key key, Action action = Action::eRelease, Mods::VALUE mods = Mods::eNONE);
	void addTrigger(std::string const& id, std::initializer_list<Trigger> triggers);
	void addState(std::string const& id, Key key);
	void addState(std::string const& id, State state);
	void addRange(std::string const& id, Axis axis, bool bReverse = false);
	void addRange(std::string const& id, Key min, Key max);

	u16 deserialise(GData const& json);
	void clear();

	u16 size() const;
	bool isEmpty() const;
};

class Context final
{
private:
	struct Callback final
	{
		OnTrigger onTrigger;
		OnState onState;
		OnRange onRange;
	};

#if defined(LEVK_DEBUG)
public:
	std::string m_name;
#endif

private:
	Map m_map;
	mutable std::unordered_set<Key> m_padHeld;
	std::unordered_map<std::string, Callback> m_callbacks;
	Mode m_mode;
	s32 m_padID;
	mutable bool m_bFired = false;

public:
	Context(Mode mode = Mode::ePassthrough, s32 padID = 0);
	Context(Context&&);
	Context& operator=(Context&&);
	Context(Context const&);
	Context& operator=(Context const&);
	~Context();

public:
	void mapTrigger(std::string const& id, OnTrigger callback);
	void mapState(std::string const& id, OnState callback);
	void mapRange(std::string const& id, OnRange callback);

	void addTrigger(std::string const& id, Key key, Action action = Action::eRelease, Mods::VALUE mods = Mods::eNONE);
	void addState(std::string const& id, Key key);
	void addState(std::string const& id, State state);
	void addRange(std::string const& id, Axis axis, bool bReverse = false);
	void addRange(std::string const& id, Key min, Key max);

	void setMode(Mode mode);
	void setGamepadID(s32 id);

	u16 deserialise(GData const& json);
	void import(Map map);

public:
	bool wasFired() const;

private:
	bool isConsumed(Snapshot const& snapshot) const;

private:
	friend void fire();
};
} // namespace le::input
