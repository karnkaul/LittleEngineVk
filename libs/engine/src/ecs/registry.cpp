#include <algorithm>
#include <map>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/ecs/component.hpp"
#include "engine/ecs/registry.hpp"

namespace le
{
std::string const s_tEName = utils::tName<Entity>();
std::string const Registry::s_tName = utils::tName<Registry>();
std::unordered_map<std::type_index, Component::Sign> Registry::s_signs;

Registry::Registry(DestroyMode destroyMode) : m_destroyMode(destroyMode)
{
	LOG_D("[{}] Constructed", s_tName);
}

Registry::~Registry()
{
	if (!m_entityFlags.empty())
	{
		LOG_I("[{}] [{}] Entities destroyed", s_tName, m_entityFlags.size());
	}
	LOG_D("[{}] Destroyed", s_tName);
}

Entity Registry::spawnEntity(std::string name)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	++m_nextID;
	LOG_I("[{}] [{}:{}] [{}] spawned", s_tName, s_tEName, m_nextID, name);
	m_entityNames[m_nextID] = std::move(name);
	m_entityFlags[m_nextID] = {};
	return {m_nextID};
}

bool Registry::destroyEntity(Entity entity)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (auto search = m_entityFlags.find(entity.id); search != m_entityFlags.end())
	{
		switch (m_destroyMode)
		{
		case DestroyMode::eImmediate:
		{
			lock.unlock();
			destroyComponents(entity);
			destroyEntity(search, entity.id);
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

bool Registry::destroyComponents(Entity entity)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (auto search = m_db.find(entity.id); search != m_db.end())
	{
		LOG_I("[{}] [{}] components detached from [{}:{}] [{}] and destroyed", s_tName, search->second.size(), s_tEName, entity.id,
			  m_entityNames[entity.id]);
		m_db.erase(search);
		return true;
	}
	return false;
}

bool Registry::setEnabled(Entity entity, bool bEnabled)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (auto search = m_entityFlags.find(entity.id); search != m_entityFlags.end())
	{
		search->second[Flag::eDisabled] = !bEnabled;
		return true;
	}
	return false;
}

bool Registry::setDebug(Entity entity, bool bDebug)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (auto search = m_entityFlags.find(entity.id); search != m_entityFlags.end())
	{
		search->second[Flag::eDebug] = !bDebug;
		return true;
	}
	return false;
}

bool Registry::isEnabled(Entity entity) const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	auto search = m_entityFlags.find(entity.id);
	return search != m_entityFlags.end() && !search->second.isSet(Flag::eDisabled);
}

bool Registry::isAlive(Entity entity) const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	auto search = m_entityFlags.find(entity.id);
	return search != m_entityFlags.end() && !search->second.isSet(Flag::eDestroyed);
}

bool Registry::isDebugSet(Entity entity) const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	auto search = m_entityFlags.find(entity.id);
	return search != m_entityFlags.end() && search->second.isSet(Flag::eDebug);
}

void Registry::cleanDestroyed()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	for (auto iter = m_entityFlags.begin(); iter != m_entityFlags.end();)
	{
		Entity const entity = {iter->first};
		auto const flags = iter->second;
		if (flags.isSet(Flag::eDestroyed))
		{
			lock.unlock();
			destroyComponents(entity);
			iter = destroyEntity(iter, entity.id);
			lock.lock();
		}
		else
		{
			++iter;
		}
	}
	return;
}

Component* Registry::attach(Component::Sign sign, std::unique_ptr<Component>&& uComp, Entity entity)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_componentNames[sign].empty())
	{
		m_componentNames[sign] = utils::tName(*uComp);
	}
	auto const id = entity.id;
	uComp->create(entity, sign);
	m_db[id][sign] = std::move(uComp);
	LOG_I("[{}] [{}] spawned and attached to [{}:{}] [{}]", s_tName, m_componentNames[sign], s_tEName, id, m_entityNames[id]);
	return m_db[id][sign].get();
}

void Registry::detach(Component const* pComponent, Entity::ID id)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	auto const sign = pComponent->m_sign;
	m_db[id].erase(sign);
	LOG_I("[{}] [{}] detached from [{}:{}] [{}] and destroyed", s_tName, m_componentNames[sign], s_tEName, id, m_entityNames[id]);
	return;
}

Registry::EFMap::iterator Registry::destroyEntity(EFMap::iterator iter, Entity::ID id)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	iter = m_entityFlags.erase(iter);
	LOG_I("[{}] [{}:{}] [{}] destroyed", s_tName, s_tEName, id, m_entityNames[id]);
	m_entityNames.erase(id);
	return iter;
}
} // namespace le
