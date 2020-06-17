#include <core/log.hpp>
#include <engine/game/world.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/levk.hpp>
#include <editor/editor.hpp>

namespace le
{
std::unordered_map<s32, std::unique_ptr<World>> World::s_worlds;
std::unordered_map<std::type_index, World*> World::s_worldByType;
World* World::s_pActive = nullptr;
World::ID World::s_lastID;

World::World() = default;
World::~World()
{
	LOG_I("[{}] World{} Destroyed", m_tName.empty() ? "UnknownWorld" : m_tName, m_id);
}

World* World::getWorld(ID id)
{
	if (auto search = s_worlds.find(id); search != s_worlds.end())
	{
		return search->second.get();
	}
	return nullptr;
}

bool World::removeWorld(ID id)
{
	if (auto search = s_worlds.find(id); search != s_worlds.end())
	{
		auto const& uWorld = search->second;
		s_worldByType.erase(uWorld->m_type);
		s_worlds.erase(search);
		return true;
	}
	return false;
}

bool World::loadWorld(ID id)
{
	if (auto search = s_worlds.find(id); search != s_worlds.end())
	{
		if (s_pActive)
		{
			s_pActive->stop();
			s_pActive = nullptr;
		}
		auto const& uWorld = search->second;
		if (uWorld->start())
		{
			s_pActive = uWorld.get();
			return true;
		}
		LOG_E("[{}] Failed to start World{}", utils::tName<World>(), id);
		return false;
	}
	LOG_E("[{}] Failed to find World{}", utils::tName<World>(), id);
	return false;
}

World::ID World::id() const
{
	return m_id;
}

std::string_view World::name() const
{
	return m_tName;
}

bool World::switchWorld(ID newWorld)
{
	return loadWorld(newWorld);
}

Entity World::spawnEntity(std::string name, bool bAddTransform)
{
	Entity ret = m_registry.spawnEntity(std::move(name));
	if (bAddTransform)
	{
		[[maybe_unused]] auto pTransform = m_registry.addComponent<Transform>(ret);
#if defined(LEVK_EDITOR)
		pTransform->setParent(&m_root);
		m_transformToEntity[pTransform] = ret;
#endif
	}
	return ret;
}

bool World::destroyEntity(Entity entity)
{
#if defined(LEVK_EDITOR)
	if (auto pTransform = m_registry.component<Transform>(entity))
	{
		m_transformToEntity.erase(pTransform);
	}
#endif
	return m_registry.destroyEntity(entity);
}

Window* World::window() const
{
	return engine::Service::mainWindow();
}

bool World::start(ID id)
{
	if (auto search = s_worlds.find(id); search != s_worlds.end())
	{
		auto const& uWorld = search->second;
		if (uWorld->start())
		{
			s_pActive = uWorld.get();
			return true;
		}
		uWorld->stop();
	}
	return false;
}

void World::tickImpl(Time dt)
{
	m_registry.sweep();
	bool bTick = true;
#if defined(LEVK_EDITOR)
	bTick = editor::g_bTickGame;
#endif
	if (bTick)
	{
		tick(dt);
	}
}

void World::stopImpl()
{
	stop();
	m_registry.clear();
}

#if defined(LEVK_EDITOR)
Registry& World::registry()
{
	return m_registry;
}
#endif

bool World::stopActive()
{
	if (s_pActive)
	{
		s_pActive->stopImpl();
		s_pActive = nullptr;
		return true;
	}
	return false;
}

void World::destroyAll()
{
	s_pActive = nullptr;
	s_worlds.clear();
	s_worldByType.clear();
}

bool World::tick(Time dt, gfx::ScreenRect const& sceneRect)
{
	if (s_pActive)
	{
		s_pActive->tickImpl(dt);
		s_pActive->m_worldRect = sceneRect;
		return true;
	}
	return false;
}

bool World::submitScene(gfx::Renderer& renderer)
{
	if (s_pActive)
	{
		renderer.submit(s_pActive->buildScene(), s_pActive->m_worldRect);
		return true;
	}
	return false;
}

World* World::active()
{
	return s_pActive;
}
} // namespace le
