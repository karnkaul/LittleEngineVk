#include <core/ecs/registry.hpp>

namespace le::ecs
{
std::string const Registry::s_tEName = utils::tName<Entity>();

Registry::Registry()
{
	m_regID = ++s_nextRegID;
	m_name = fmt::format("{}:{}", utils::tName(this), m_regID);
}

Registry::~Registry()
{
	clear();
}

bool Registry::destroy(Entity entity)
{
	auto lock = m_mutex.lock();
	bool bRet = false;
	std::string name;
	auto& storage = get_Impl<Info>();
	if (auto pInfo = storage.find(entity))
	{
		name = pInfo->name;
	}
	for (auto& [_, uConcept] : m_db)
	{
		bRet |= uConcept->detach(entity);
	}
	LOGIF(bRet && m_logLevel && !name.empty(), *m_logLevel, "[{}] [{}:{}] [{}] destroyed", m_name, s_tEName, entity.id, name);
	return bRet;
}

bool Registry::enable(Entity entity, bool bEnabled)
{
	auto lock = m_mutex.lock();
	if (auto pInfo = get_Impl<Info>().find(entity))
	{
		pInfo->flags[Flag::eDisabled] = !bEnabled;
		return true;
	}
	return false;
}

bool Registry::enabled(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto pStorage = cast_Impl<Info>(); auto pInfo = pStorage->find(entity))
	{
		return !pInfo->flags.test(Flag::eDisabled);
	}
	return false;
}

bool Registry::exists(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto pStorage = cast_Impl<Info>())
	{
		return pStorage->find(entity) != nullptr;
	}
	return false;
}

std::string_view Registry::name(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto pStorage = cast_Impl<Info>(); auto pInfo = pStorage->find(entity))
	{
		return pInfo->name;
	}
	return {};
}

Registry::Info* Registry::info(Entity entity)
{
	auto lock = m_mutex.lock();
	auto& storage = get_Impl<Info>();
	if (auto pInfo = storage.find(entity))
	{
		return pInfo;
	}
	return nullptr;
}

Registry::Info const* Registry::info(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto pStorage = cast_Impl<Info>())
	{
		return pStorage->find(entity);
	}
	return nullptr;
}

std::size_t Registry::size() const
{
	auto lock = m_mutex.lock();
	if (auto pStorage = cast_Impl<Info>())
	{
		return pStorage->size();
	}
	return 0;
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

bool Registry::exists_Impl(Sign sign, Entity entity) const
{
	if (auto search = m_db.find(sign); search != m_db.end())
	{
		return search->second->exists(entity);
	}
	return false;
}

Entity Registry::spawn_Impl(std::string name)
{
	auto const id = ++m_nextID;
	Entity ret{id, m_regID};
	auto& info = attach_Impl<Info>(ret, {});
	info.name = std::move(name);
	LOGIF(m_logLevel, *m_logLevel, "[{}] [{}:{}] [{}] spawned", m_name, s_tEName, id, info.name);
	return ret;
}
} // namespace le::ecs
