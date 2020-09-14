#include <algorithm>
#include <map>
#include <core/log.hpp>
#include <core/ecs_registry.hpp>

namespace le
{
std::string const s_tEName = utils::tName<Entity>();

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

Registry::Concept::~Concept() = default;

Registry::Registry(DestroyMode destroyMode) : m_destroyMode(destroyMode)
{
	m_regID = ++s_nextRegID.payload;
	m_name = fmt::format("{}:{}", utils::tName(*this), m_regID);
}

Registry::~Registry()
{
	if (!m_db.empty())
	{
		LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] Entities destroyed", m_name, m_db.size());
	}
}

Entity Registry::spawn(std::string name)
{
	auto lock = m_mutex.lock();
	return spawn_Impl(std::move(name));
}

bool Registry::enable(Entity entity, bool bEnabled)
{
	auto lock = m_mutex.lock();
	if (auto pInfo = find_Impl<Registry, Info>(this, entity))
	{
		pInfo->flags[Flag::eDisabled] = !bEnabled;
	}
	return false;
}

bool Registry::enabled(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto pInfo = find_Impl<Registry const, Info const>(this, entity))
	{
		return !pInfo->flags.test(Flag::eDisabled);
	}
	return false;
}

bool Registry::exists(Entity entity) const
{
	auto lock = m_mutex.lock();
	return m_db.find(entity) != m_db.end();
}

std::string_view Registry::name(Entity entity) const
{
	if (auto pInfo = find<Info>(entity))
	{
		return pInfo->name;
	}
	return {};
}

Registry::Info* Registry::info(Entity entity)
{
	if (auto pInfo = find<Info>(entity))
	{
		return pInfo;
	}
	return nullptr;
}

Registry::Info const* Registry::info(Entity entity) const
{
	if (auto pInfo = find<Info>(entity))
	{
		return pInfo;
	}
	return nullptr;
}

bool Registry::destroy(Entity const& entity)
{
	auto lock = m_mutex.lock();
	if (auto search = m_db.find(entity); search != m_db.end())
	{
		switch (m_destroyMode)
		{
		case DestroyMode::eImmediate:
		{
			destroy_Impl(search, entity);
			break;
		}
		default:
		case DestroyMode::eDeferred:
		{
			if (auto pInfo = find_Impl<Registry, Info>(this, entity))
			{
				pInfo->flags.set(Flag::eDestroyed);
			}
			else
			{
				ASSERT(false, "Invariant violated!");
			}
			break;
		}
		}
		return true;
	}
	return false;
}

bool Registry::destroy(Entity& out_entity)
{
	Entity const& entity = out_entity;
	if (destroy(entity))
	{
		out_entity = {};
		return true;
	}
	return false;
}

void Registry::flush()
{
	auto lock = m_mutex.lock();
	for (auto iter = m_db.begin(); iter != m_db.end();)
	{
		bool bInc = true;
		Entity const entity = {iter->first};
		auto& compMap = iter->second;
		if (auto pInfo = find_Impl<Info>(compMap))
		{
			if (pInfo->flags.test(Flag::eDestroyed))
			{
				iter = destroy_Impl(iter, entity);
				bInc = false;
			}
		}
		if (bInc)
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
}

std::size_t Registry::size() const
{
	auto lock = m_mutex.lock();
	return m_db.size();
}

std::string_view Registry::name_Impl(Signature sign)
{
	if (auto search = s_names.find(sign); search != s_names.end())
	{
		return search->second;
	}
	return {};
}

Entity Registry::spawn_Impl(std::string name)
{
	++m_nextID.payload;
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}:{}] [{}] spawned", m_name, s_tEName, m_nextID, name);
	Entity ret{m_nextID, m_regID};
	if (auto pInfo = attach_Impl<Info>(ret))
	{
		pInfo->name = std::move(name);
		return ret;
	}
	return {};
}

Registry::Concept* Registry::attach_Impl(Signature sign, std::unique_ptr<Concept>&& uComp, Entity entity)
{
	uComp->sign = sign;
	auto& compMap = m_db[entity];
	ASSERT(compMap.find(sign) == compMap.end(), "Duplicate component!");
	if (m_logLevel)
	{
		if (auto pInfo = find_Impl<Info>(compMap))
		{
			LOG(*m_logLevel, "[{}] [{}] spawned and attached to [{}:{}] [{}]", m_name, name_Impl(sign), s_tEName, entity.id, pInfo->name);
		}
	}
	auto& comp = compMap[sign];
	comp = std::move(uComp);
	return comp.get();
}

Registry::ECMap::iterator Registry::destroy_Impl(ECMap::iterator iter, Entity entity)
{
	if (m_logLevel)
	{
		if (auto pInfo = find_Impl<Registry, Info>(this, entity))
		{
			LOG(*m_logLevel, "[{}] [{}:{}] [{}] destroyed", m_name, s_tEName, entity.id, pInfo->name);
		}
	}
	return m_db.erase(iter);
}

Registry::CompMap::iterator Registry::detach_Impl(CompMap& out_map, CompMap::iterator iter, Entity entity, Signature sign)
{
	if (m_logLevel)
	{
		if (auto pInfo = find_Impl<Registry, Info>(this, entity))
		{
			LOG(*m_logLevel, "[{}] [{}] detached from [{}:{}] [{}] and destroyed", m_name, name_Impl(sign), s_tEName, entity.id, pInfo->name);
		}
	}
	return out_map.erase(iter);
}
} // namespace le
