#include <algorithm>
#include <map>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <engine/ecs/registry.hpp>

namespace le
{
bool g_bDestroyed = false;

std::string const s_tEName = utils::tName<Entity>();
std::unordered_map<std::type_index, Registry::Signature> Registry::s_signs;
Lockable<std::mutex> Registry::s_mutex;
ECSID Registry::s_nextRegID;

bool operator==(Entity lhs, Entity rhs)
{
	return lhs.id == rhs.id;
}

bool operator!=(Entity lhs, Entity rhs)
{
	return !(lhs == rhs);
}

std::size_t EntityHasher::operator()(Entity const& entity) const
{
	return std::hash<ECSID>()(entity.id) ^ std::hash<ECSID>()(entity.regID);
}

Registry::Component::~Component() = default;

Registry::Registry(DestroyMode destroyMode) : m_destroyMode(destroyMode)
{
	m_regID = ++s_nextRegID.payload;
	m_name = fmt::format("{}:{}", utils::tName(*this), m_regID);
	LOG_D("[{}] Constructed", m_name);
}

Registry::~Registry()
{
	if (!m_entityFlags.empty())
	{
		LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] Entities destroyed", m_name, m_entityFlags.size());
	}
	LOG_D("[{}] Destroyed", m_name);
	g_bDestroyed = true;
}

bool Registry::setEnabled(Entity entity, bool bEnabled)
{
	auto lock = m_mutex.lock();
	if (auto search = m_entityFlags.find(entity); search != m_entityFlags.end())
	{
		search->second[Flag::eDisabled] = !bEnabled;
		return true;
	}
	return false;
}

bool Registry::setDebug(Entity entity, bool bDebug)
{
	auto lock = m_mutex.lock();
	if (auto search = m_entityFlags.find(entity); search != m_entityFlags.end())
	{
		search->second[Flag::eDebug] = !bDebug;
		return true;
	}
	return false;
}

bool Registry::isEnabled(Entity entity) const
{
	auto lock = m_mutex.lock();
	auto search = m_entityFlags.find(entity);
	return search != m_entityFlags.end() && !search->second.isSet(Flag::eDisabled);
}

bool Registry::isAlive(Entity entity) const
{
	auto lock = m_mutex.lock();
	auto search = m_entityFlags.find(entity);
	return search != m_entityFlags.end() && !search->second.isSet(Flag::eDestroyed);
}

bool Registry::isDebugSet(Entity entity) const
{
	auto lock = m_mutex.lock();
	auto search = m_entityFlags.find(entity);
	return search != m_entityFlags.end() && search->second.isSet(Flag::eDebug);
}

bool Registry::destroyEntity(Entity const& entity)
{
	auto lock = m_mutex.lock();
	if (auto search = m_entityFlags.find(entity); search != m_entityFlags.end())
	{
		switch (m_destroyMode)
		{
		case DestroyMode::eImmediate:
		{
			destroyComponent_Impl(entity);
			destroyEntity_Impl(search, entity);
			break;
		}
		default:
		case DestroyMode::eDeferred:
		{
			search->second.set(Flag::eDestroyed);
			break;
		}
		}
		return true;
	}
	return false;
}

bool Registry::destroyEntity(Entity& out_entity)
{
	Entity const& entity = out_entity;
	if (destroyEntity(entity))
	{
		out_entity = {};
		return true;
	}
	return false;
}

void Registry::flush()
{
	auto lock = m_mutex.lock();
	for (auto iter = m_entityFlags.begin(); iter != m_entityFlags.end();)
	{
		Entity const entity = {iter->first};
		auto const flags = iter->second;
		if (flags.isSet(Flag::eDestroyed))
		{
			destroyComponent_Impl(entity);
			iter = destroyEntity_Impl(iter, entity);
		}
		else
		{
			++iter;
		}
	}
	return;
}

void Registry::clear()
{
	auto lock = m_mutex.lock();
	m_db.clear();
	m_entityFlags.clear();
	m_entityNames.clear();
	m_componentNames.clear();
}

std::size_t Registry::entityCount() const
{
	auto lock = m_mutex.lock();
	return m_entityFlags.size();
}

std::string_view Registry::entityName(Entity entity) const
{
	auto lock = m_mutex.lock();
	auto search = m_entityNames.find(entity);
	return search != m_entityNames.end() ? search->second : std::string_view();
}

Entity Registry::spawnEntity_Impl(std::string name)
{
	++m_nextID.payload;
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}:{}] [{}] spawned", m_name, s_tEName, m_nextID, name);
	Entity ret{m_nextID, m_regID};
	m_entityNames[ret] = std::move(name);
	m_entityFlags[ret] = {};
	return ret;
}

Registry::EFMap::iterator Registry::destroyEntity_Impl(EFMap::iterator iter, Entity entity)
{
	iter = m_entityFlags.erase(iter);
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}:{}] [{}] destroyed", m_name, s_tEName, entity.id, m_entityNames[entity]);
	m_entityNames.erase(entity);
	return iter;
}

Registry::Component* Registry::addComponent_Impl(Signature sign, std::unique_ptr<Component>&& uComp, Entity entity)
{
	if (m_componentNames[sign].empty())
	{
		m_componentNames[sign] = utils::tName(*uComp);
	}
	uComp->sign = sign;
	ASSERT(m_db[entity].find(sign) == m_db[entity].end(), "Duplicate Component!");
	m_db[entity][sign] = std::move(uComp);
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] spawned and attached to [{}:{}] [{}]", m_name, m_componentNames[sign], s_tEName, entity.id,
		  m_entityNames[entity]);
	return m_db[entity][sign].get();
}

void Registry::destroyComponent_Impl(Component const* pComponent, Entity entity)
{
	ASSERT(!g_bDestroyed, "Registry destroyed!");
	auto const sign = pComponent->sign;
	m_db[entity].erase(sign);
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] detached from [{}:{}] [{}] and destroyed", m_name, m_componentNames[sign], s_tEName, entity.id,
		  m_entityNames[entity]);
	return;
}

bool Registry::destroyComponent_Impl(Entity entity)
{
	ASSERT(!g_bDestroyed, "Registry destroyed!");
	if (auto search = m_db.find(entity); search != m_db.end())
	{
		LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] components detached from [{}:{}] [{}] and destroyed", m_name, search->second.size(), s_tEName, entity.id,
			  m_entityNames[entity]);
		m_db.erase(search);
		return true;
	}
	return false;
}
} // namespace le
