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

Registry::Registry()
{
	m_regID = ++s_nextRegID;
	m_name = fmt::format("{}:{}", utils::tName(*this), m_regID);
}

Registry::~Registry()
{
	clear();
}

bool Registry::destroy(Entity entity)
{
	auto lock = m_mutex.lock();
	auto const infoSign = sign_Impl<Info>();
	bool bRet = false;
	std::string name;
	for (auto& [s, emap] : m_db)
	{
		if (auto search = emap.find(entity); search != emap.end())
		{
			if (s == infoSign)
			{
				auto const& info = static_cast<Model<Info>&>(*search->second).t;
				name = std::move(info.name);
			}
			detach_Impl(emap, search, s, {});
			bRet = true;
		}
	}
	LOGIF(bRet && m_logLevel && !name.empty(), *m_logLevel, "[{}] [{}:{}] [{}] destroyed", m_name, s_tEName, entity.id, name);
	return bRet;
}

bool Registry::enable(Entity entity, bool bEnabled)
{
	auto lock = m_mutex.lock();
	if (auto pInfo = find_Impl<Info>(this, m_db[sign_Impl<Info>()], entity))
	{
		pInfo->flags[Flag::eDisabled] = !bEnabled;
		return true;
	}
	return false;
}

bool Registry::enabled(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto search = m_db.find(sign_Impl<Info>()); search != m_db.end())
	{
		if (auto pInfo = find_Impl<Info const>(this, search->second, entity))
		{
			return !pInfo->flags.test(Flag::eDisabled);
		}
	}
	return false;
}

bool Registry::exists(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto search = m_db.find(sign_Impl<Info>()); search != m_db.end())
	{
		return search->second.find(entity) != search->second.end();
	}
	return false;
}

std::string_view Registry::name(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto pInfo = find_Impl<Info>(this, entity))
	{
		return pInfo->name;
	}
	return {};
}

Registry::Info* Registry::info(Entity entity)
{
	auto lock = m_mutex.lock();
	if (auto pInfo = find_Impl<Info>(this, entity))
	{
		return pInfo;
	}
	return nullptr;
}

Registry::Info const* Registry::info(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto pInfo = find_Impl<Info>(this, entity))
	{
		return pInfo;
	}
	return nullptr;
}

void Registry::clear()
{
	auto const s = size();
	auto lock = m_mutex.lock();
	if (!m_db.empty())
	{
		LOGIF(m_logLevel, *m_logLevel, "[{}] [{}] Entities and [{}] Component tables destroyed", m_name, s, m_db.size());
		m_db.clear();
	}
}

std::size_t Registry::size() const
{
	auto lock = m_mutex.lock();
	if (auto search = m_db.find(sign_Impl<Info>()); search != m_db.end())
	{
		return search->second.size();
	}
	return 0;
}

std::string_view Registry::name_Impl(Sign sign)
{
	if (auto search = s_names.find(sign); search != s_names.end())
	{
		return search->second;
	}
	return {};
}

Entity Registry::spawn_Impl(std::string name)
{
	auto const id = ++m_nextID;
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}:{}] [{}] spawned", m_name, s_tEName, id, name);
	Entity ret{id, m_regID};
	auto& info = attach_Impl<Info>(ret, m_db[sign_Impl<Info>()]);
	info.name = std::move(name);
	return ret;
}

Registry::Concept* Registry::attach_Impl(Sign s, EMap& out_emap, std::unique_ptr<Concept>&& uComp, Entity entity)
{
	uComp->sign = s;
	ASSERT(out_emap.find(entity) == out_emap.end(), "Duplicate component!");
	if (m_logLevel)
	{
		if (auto pInfo = find_Impl<Info>(this, m_db[sign<Info>()], entity))
		{
			LOG(*m_logLevel, "[{}] [{}] attached to [{}:{}] [{}]", m_name, name_Impl(s), s_tEName, entity.id, pInfo->name);
		}
	}
	auto& comp = out_emap[entity];
	comp = std::move(uComp);
	return comp.get();
}

Registry::EMap::iterator Registry::detach_Impl(EMap& out_emap, EMap::iterator iter, Sign sign, std::string_view name)
{
	if (m_logLevel && !name.empty())
	{
		LOG(*m_logLevel, "[{}] [{}] detached from [{}:{}] [{}] and destroyed", m_name, name_Impl(sign), s_tEName, iter->first.id, name);
	}
	return out_emap.erase(iter);
}

bool Registry::exists_Impl(Sign s, Entity entity) const
{
	if (auto cSearch = m_db.find(s); cSearch != m_db.end())
	{
		auto& eList = cSearch->second;
		return eList.find(entity) != eList.end();
	}
	return false;
}
} // namespace le
