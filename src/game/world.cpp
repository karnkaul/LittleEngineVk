#include <core/log.hpp>
#include <engine/game/world.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/levk.hpp>
#include <game/input_impl.hpp>
#include <resources/manifest.hpp>
#include <levk_impl.hpp>

namespace le
{
namespace
{
struct
{
	res::ResourceList loadedResources;
	World* pNext = nullptr;
} g_data;

res::Manifest g_manifest;
} // namespace

World::World() = default;
World::~World()
{
	LOG_I("[{}] World{} Destroyed", m_name.empty() ? "UnknownWorld" : m_name, m_id);
}

World* World::world(ID id)
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
	if (isBusy())
	{
		LOG_W("[{}] Busy, ignoring request to load World{}", utils::tName<World>(), id);
		return false;
	}
	if (auto search = s_worlds.find(id); search != s_worlds.end())
	{
		g_data.pNext = search->second.get();
		return true;
	}
	LOG_E("[{}] Failed to find World{}", utils::tName<World>(), id);
	return false;
}

World* World::active()
{
	return s_pActive;
}

World::Semaphore World::setBusy()
{
	return s_busyCounter;
}

bool World::isBusy()
{
	return !g_manifest.isIdle() || !s_busyCounter.isZero(true);
}

bool World::worldLoadPending()
{
	return g_data.pNext != nullptr;
}

#if defined(LEVK_EDITOR)
std::vector<World*> World::allWorlds()
{
	std::vector<World*> ret;
	for (auto& [id, uWorld] : s_worlds)
	{
		ret.push_back(uWorld.get());
	}
	std::sort(ret.begin(), ret.end(), [](auto pLHS, auto pRHS) { return pLHS->m_id < pRHS->m_id; });
	return ret;
}
#endif

World::ID World::id() const
{
	return m_id;
}

std::string_view World::name() const
{
	return m_name;
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

stdfs::path World::manifestID() const
{
	static stdfs::path const s_empty;
	return s_empty;
}

stdfs::path World::inputMapID() const
{
	static stdfs::path const s_empty;
	return s_empty;
}

void World::onManifestLoaded() {}

#if defined(LEVK_EDITOR)
Registry& World::registry()
{
	return m_registry;
}
#endif

bool World::impl_start(ID previous)
{
	m_previousWorldID = previous;
	m_inputContext = {};
#if defined(LEVK_DEBUG)
	m_inputContext.context.m_name = m_name;
#endif
	auto const inputMap = inputMapID();
	if (!inputMap.empty() && engine::reader().isPresent(inputMap))
	{
		if (auto str = engine::reader().string(inputMap))
		{
			dj::object json;
			if (json.read(*str))
			{
				if (auto const parsed = m_inputContext.context.deserialise(json); parsed > 0)
				{
					LOG_D("[{}] Parsed [{}] input mappings from [{}]", m_name, parsed, inputMap.generic_string());
				}
			}
			else
			{
				LOG_W("[{}] Failed to read input mappings from [{}]!", m_name, inputMap.generic_string());
			}
		}
	}
	input::registerContext(m_inputContext);
	if (start())
	{
		LOG_I("[{}] started", utils::tName(*this));
		return true;
	}
	m_previousWorldID = {};
	m_inputContext.token.reset();
	return false;
}

void World::impl_tick(gfx::ScreenRect const& worldRect, Time dt, bool bTickSelf)
{
	m_registry.flush();
	if (bTickSelf)
	{
		m_worldRect = worldRect;
		tick(dt);
	}
}

void World::impl_stop()
{
	m_inputContext.token.reset();
	g_manifest.reset();
	stop();
	m_registry.clear();
	LOG_I("[{}] stopped", utils::tName(*this));
}

bool World::impl_start(World& out_world, ID prev)
{
	if (out_world.impl_start(prev))
	{
		s_pActive = &out_world;
		if (g_manifest.isReady())
		{
			g_manifest.start();
		}
		return true;
	}
	out_world.impl_stop();
	LOG_E("[{}] Failed to start World{}", utils::tName<World>(), out_world.id());
	return false;
}

bool World::impl_startID(ID id)
{
	input::g_bFire = true;
	if (auto search = s_worlds.find(id); search != s_worlds.end())
	{
		auto const& uWorld = search->second;
		auto const manifestID = uWorld->manifestID();
		if (!manifestID.empty() && engine::reader().isPresent(manifestID))
		{
			g_manifest.read(manifestID);
		}
		return impl_start(*uWorld, {});
	}
	LOG_E("[{}] Failed to find World{}!", utils::tName<World>(), id);
	return false;
}

void World::impl_startNext()
{
	ID previousID;
	bool bSkipUnload = false;
	if (s_pActive)
	{
		previousID = s_pActive->m_id;
		s_pActive->impl_stop();
		bSkipUnload = s_pActive->m_flags.isSet(Flag::eSkipManifestUnload);
		s_pActive = nullptr;
	}
	auto const manifestID = g_data.pNext->manifestID();
	auto toUnload = g_data.loadedResources;
	if (!manifestID.empty() && engine::reader().isPresent(manifestID))
	{
		g_manifest.read(manifestID);
		auto const loadList = g_manifest.parse();
		auto const toLoad = loadList - g_data.loadedResources;
		toUnload = g_data.loadedResources - loadList;
		g_manifest.m_toLoad.intersect(toLoad);
	}
	if (bSkipUnload)
	{
		toUnload = {};
	}
	res::Manifest::unload(toUnload);
	g_data.loadedResources = g_data.loadedResources - toUnload;
	impl_start(*g_data.pNext, previousID);
	g_data.pNext = nullptr;
}

bool World::impl_stopActive()
{
	if (s_pActive)
	{
		s_pActive->impl_stop();
		s_pActive = nullptr;
		return true;
	}
	return false;
}

void World::impl_destroyAll()
{
	impl_stopActive();
	g_data = {};
	g_manifest.reset();
	s_pActive = nullptr;
	s_worlds.clear();
	s_worldByType.clear();
}

bool World::impl_tick(Time dt, gfx::ScreenRect const& sceneRect, bool bTickActive, bool bTerminate)
{
	if (!engine::mainWindow())
	{
		return false;
	}
	if (g_data.pNext && g_manifest.isIdle())
	{
		impl_startNext();
	}
	if (!g_manifest.isIdle())
	{
		auto const status = g_manifest.update(bTerminate);
		switch (status)
		{
		case res::Manifest::Status::eIdle:
		{
			if (s_pActive)
			{
				s_pActive->onManifestLoaded();
			}
			g_data.loadedResources = std::move(g_manifest.m_loaded);
			g_manifest.reset();
			break;
		}
		default:
		{
			break;
		}
		}
	}
	if (s_pActive)
	{
		s_pActive->impl_tick(sceneRect, dt, bTickActive);
		return true;
	}
	return false;
}

bool World::impl_submitScene(gfx::Renderer& out_renderer, gfx::Camera const& camera)
{
	if (s_pActive)
	{
		auto const builder = s_pActive->sceneBuilder();
		out_renderer.submit(builder.build(camera, s_pActive->m_registry), s_pActive->m_worldRect);
		return true;
	}
	return false;
}
} // namespace le
