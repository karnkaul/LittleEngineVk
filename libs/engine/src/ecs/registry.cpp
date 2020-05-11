#include <algorithm>
#include <map>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/ecs/component.hpp"
#include "engine/ecs/registry.hpp"

namespace le::ecs
{
std::string const Entity::s_tName = utils::tName<Entity>();
std::string const Registry::s_tName = utils::tName<Registry>();

Registry::Registry(DestroyMode destroyMode) : m_destroyMode(destroyMode)
{
	LOG_D("[{}] Constructed", s_tName);
}

Registry::Registry(Registry&&) = default;
Registry& Registry::operator=(Registry&&) = default;

Registry::~Registry()
{
	if (!m_entities.empty())
	{
		LOG_I("[{}] [{}] Entities destroyed", s_tName, m_entities.size());
	}
	LOG_D("[{}] Destroyed", s_tName);
}

Entity::ID Registry::spawnEntity(std::string name)
{
	Entity::nextID(&m_nextID);
	auto& ret = m_entities[m_nextID];
	ret.m_id = m_nextID;
	LOG_I("[{}] [{}:{}] [{}] spawned", s_tName, Entity::s_tName, ret.m_id, name);
	m_entityNames[ret.m_id] = std::move(name);
	return ret.m_id;
}

Entity* Registry::entity(Entity::ID id)
{
	auto search = m_entities.find(id);
	return search != m_entities.end() ? &search->second : nullptr;
}

Entity const* Registry::entity(Entity::ID id) const
{
	auto search = m_entities.find(id);
	return search != m_entities.end() ? &search->second : nullptr;
}

bool Registry::destroyEntity(Entity::ID id)
{
	switch (m_destroyMode)
	{
	case DestroyMode::eImmediate:
	{
		if (auto search = m_entities.find(id); search != m_entities.end())
		{
			destroyComponents(id);
			destroyEntity(search, id);
			return true;
		}
		return false;
	}
	default:
	case DestroyMode::eDeferred:
	{
		if (auto search = m_entityFlags.find(id); search != m_entityFlags.end())
		{
			search->second.set(EntityFlag::eDestroyed);
			return true;
		}
		return false;
	}
	}
}

bool Registry::destroyComponents(Entity::ID id)
{
	auto search = m_db.find(id);
	if (search != m_db.end())
	{
		LOG_I("[{}] [{}] components detached from [{}:{}] [{}] and destroyed", s_tName, search->second.size(), Entity::s_tName, id,
			  m_entityNames[id]);
		m_db.erase(search);
		return true;
	}
	return false;
}

bool Registry::setEnabled(Entity::ID id, bool bEnabled)
{
	auto search = m_entityFlags.find(id);
	if (search != m_entityFlags.end())
	{
		search->second[EntityFlag::eDisabled] = !bEnabled;
		return true;
	}
	return false;
}

bool Registry::isEnabled(Entity::ID id) const
{
	auto search = m_entityFlags.find(id);
	return search != m_entityFlags.end() && !search->second.isSet(EntityFlag::eDisabled);
}

bool Registry::isAlive(Entity::ID id) const
{
	auto search = m_entityFlags.find(id);
	return search != m_entityFlags.end() && !search->second.isSet(EntityFlag::eDestroyed);
}

void Registry::cleanDestroyed()
{
	for (auto iter = m_entities.begin(); iter != m_entities.end();)
	{
		auto const id = iter->second.m_id;
		if (m_entityFlags[id].isSet(EntityFlag::eDestroyed))
		{
			destroyComponents(id);
			iter = destroyEntity(iter, id);
		}
		else
		{
			++iter;
		}
	}
	return;
}

Component* Registry::attach(Component::Sign sign, std::unique_ptr<Component>&& uComp, Entity* pEntity)
{
	if (m_componentNames[sign].empty())
	{
		m_componentNames[sign] = utils::tName(*uComp);
	}
	auto const id = pEntity->m_id;
	uComp->create(pEntity, sign);
	m_db[id][sign] = std::move(uComp);
	LOG_I("[{}] [{}] spawned and attached to [{}:{}] [{}]", s_tName, m_componentNames[sign], Entity::s_tName, id, m_entityNames[id]);
	return m_db[id][sign].get();
}

void Registry::detach(Component const* pComponent, Entity::ID id)
{
	auto const sign = pComponent->m_sign;
	m_db[id].erase(sign);
	LOG_I("[{}] [{}] detached from [{}:{}] [{}] and destroyed", s_tName, m_componentNames[sign], Entity::s_tName, id, m_entityNames[id]);
	return;
}

Registry::EntityMap::iterator Registry::destroyEntity(EntityMap::iterator iter, Entity::ID id)
{
	iter = m_entities.erase(iter);
	LOG_I("[{}] [{}:{}] [{}] destroyed", s_tName, Entity::s_tName, id, m_entityNames[id]);
	m_entityNames.erase(id);
	m_entityFlags.erase(id);
	return iter;
}
} // namespace le::ecs
