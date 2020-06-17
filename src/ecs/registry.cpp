#include <algorithm>
#include <map>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <engine/ecs/registry.hpp>

namespace le
{
std::string const s_tEName = utils::tName<Entity>();
std::string const Registry::s_tName = utils::tName<Registry>();
std::unordered_map<std::type_index, Registry::Signature> Registry::s_signs;
std::mutex Registry::s_mutex;

bool operator==(Entity lhs, Entity rhs)
{
	return lhs.id == rhs.id;
}

bool operator!=(Entity lhs, Entity rhs)
{
	return !(lhs == rhs);
}

Registry::Component::~Component() = default;

Registry::Registry(DestroyMode destroyMode) : m_destroyMode(destroyMode)
{
	LOG_D("[{}] Constructed", s_tName);
}

Registry::~Registry()
{
	if (!m_entityFlags.empty())
	{
		LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] Entities destroyed", s_tName, m_entityFlags.size());
	}
	LOG_D("[{}] Destroyed", s_tName);
}

bool Registry::setEnabled(Entity entity, bool bEnabled)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	if (auto search = m_entityFlags.find(entity.id); search != m_entityFlags.end())
	{
		search->second[Flag::eDisabled] = !bEnabled;
		return true;
	}
	return false;
}

bool Registry::setDebug(Entity entity, bool bDebug)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	if (auto search = m_entityFlags.find(entity.id); search != m_entityFlags.end())
	{
		search->second[Flag::eDebug] = !bDebug;
		return true;
	}
	return false;
}

bool Registry::isEnabled(Entity entity) const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	auto search = m_entityFlags.find(entity.id);
	return search != m_entityFlags.end() && !search->second.isSet(Flag::eDisabled);
}

bool Registry::isAlive(Entity entity) const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	auto search = m_entityFlags.find(entity.id);
	return search != m_entityFlags.end() && !search->second.isSet(Flag::eDestroyed);
}

bool Registry::isDebugSet(Entity entity) const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	auto search = m_entityFlags.find(entity.id);
	return search != m_entityFlags.end() && search->second.isSet(Flag::eDebug);
}

bool Registry::destroyEntity(Entity const& entity)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	if (auto search = m_entityFlags.find(entity.id); search != m_entityFlags.end())
	{
		switch (m_destroyMode)
		{
		case DestroyMode::eImmediate:
		{
			destroyComponent_Impl(entity.id);
			destroyEntity_Impl(search, entity.id);
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

void Registry::sweep()
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	for (auto iter = m_entityFlags.begin(); iter != m_entityFlags.end();)
	{
		Entity const entity = {iter->first};
		auto const flags = iter->second;
		if (flags.isSet(Flag::eDestroyed))
		{
			destroyComponent_Impl(entity.id);
			iter = destroyEntity_Impl(iter, entity.id);
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
	std::scoped_lock<std::mutex> lock(m_mutex);
	m_db.clear();
	m_entityFlags.clear();
	m_entityNames.clear();
	m_componentNames.clear();
}

size_t Registry::entityCount() const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	return m_entityFlags.size();
}

std::string_view Registry::entityName(Entity entity) const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	auto search = m_entityNames.find(entity.id);
	return search != m_entityNames.end() ? search->second : std::string_view();
}

Entity Registry::spawnEntity_Impl(std::string name)
{
	++m_nextID;
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}:{}] [{}] spawned", s_tName, s_tEName, m_nextID, name);
	m_entityNames[m_nextID] = std::move(name);
	m_entityFlags[m_nextID] = {};
	return {m_nextID};
}

Registry::EFMap::iterator Registry::destroyEntity_Impl(EFMap::iterator iter, Entity::ID id)
{
	iter = m_entityFlags.erase(iter);
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}:{}] [{}] destroyed", s_tName, s_tEName, id, m_entityNames[id]);
	m_entityNames.erase(id);
	return iter;
}

Registry::Component* Registry::addComponent_Impl(Signature sign, std::unique_ptr<Component>&& uComp, Entity entity)
{
	if (m_componentNames[sign].empty())
	{
		m_componentNames[sign] = utils::tName(*uComp);
	}
	auto const id = entity.id;
	uComp->sign = sign;
	ASSERT(m_db[id].find(sign) == m_db[id].end(), "Duplicate Component!");
	m_db[id][sign] = std::move(uComp);
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] spawned and attached to [{}:{}] [{}]", s_tName, m_componentNames[sign], s_tEName, id,
		  m_entityNames[id]);
	return m_db[id][sign].get();
}

void Registry::destroyComponent_Impl(Component const* pComponent, Entity::ID id)
{
	auto const sign = pComponent->sign;
	m_db[id].erase(sign);
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] detached from [{}:{}] [{}] and destroyed", s_tName, m_componentNames[sign], s_tEName, id,
		  m_entityNames[id]);
	return;
}

bool Registry::destroyComponent_Impl(Entity::ID id)
{
	if (auto search = m_db.find(id); search != m_db.end())
	{
		LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] components detached from [{}:{}] [{}] and destroyed", s_tName, search->second.size(),
			  s_tEName, id, m_entityNames[id]);
		m_db.erase(search);
		return true;
	}
	return false;
}
} // namespace le
