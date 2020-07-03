#include <core/log.hpp>
#include <engine/game/world.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/levk.hpp>
#include <editor/editor.hpp>
#include <assets/manifest.hpp>
#include <levk_impl.hpp>

namespace le
{
namespace
{
std::unique_ptr<AssetManifest> g_uManifest;
AssetList g_loadedAssets;
World* g_pWaitingToStart = nullptr;
} // namespace

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
		g_pWaitingToStart = search->second.get();
		return true;
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

bool World::start(ID id)
{
	if (auto search = s_worlds.find(id); search != s_worlds.end())
	{
		auto const& uWorld = search->second;
		auto const manifestID = uWorld->manifestID();
		if (engine::reader().isPresent(manifestID))
		{
			g_uManifest = std::make_unique<AssetManifest>(engine::reader(), manifestID);
		}
		if (uWorld->startImpl())
		{
			s_pActive = uWorld.get();
			return true;
		}
		uWorld->stopImpl();
		LOG_E("[{}] Failed to start World{}!", utils::tName<World>(), id);
		return false;
	}
	LOG_E("[{}] Failed to find World{}!", utils::tName<World>(), id);
	return false;
}

bool World::startImpl(ID previous)
{
	m_previousWorldID = previous;
	m_inputContext = {};
#if defined(LEVK_DEBUG)
	m_inputContext.context.m_name = m_tName;
#endif
	auto const inputMap = inputMapID();
	if (!inputMap.empty() && engine::reader().isPresent(inputMap))
	{
		auto [str, bResult] = engine::reader().getString(inputMap);
		if (bResult)
		{
			GData json;
			if (json.read(std::move(str)))
			{
				if (auto const parsed = m_inputContext.context.deserialise(json); parsed > 0)
				{
					LOG_D("[{}] Parsed [{}] input mappings from [{}]", m_tName, parsed, inputMap.generic_string());
				}
			}
			else
			{
				LOG_W("[{}] Failed to read input mappings from [{}]!", m_tName, inputMap.generic_string());
			}
		}
	}
	input::registerContext(m_inputContext);
	if (start())
	{
		if (g_uManifest)
		{
			g_uManifest->start();
		}
		LOG_I("[{}] started", utils::tName(*this));
		return true;
	}
	m_previousWorldID = {};
	m_inputContext.token.reset();
	return false;
}

void World::tickImpl(Time dt)
{
	m_registry.flush();
	bool bTick = !engine::mainWindow()->isClosing();
	if (g_uManifest)
	{
		auto const status = g_uManifest->update(!bTick);
		switch (status)
		{
		case AssetManifest::Status::eIdle:
		{
			if (bTick)
			{
				onManifestLoaded();
			}
			g_loadedAssets = std::move(g_uManifest->m_loaded);
			g_uManifest.reset();
			break;
		}
		default:
		{
			break;
		}
		}
	}
#if defined(LEVK_EDITOR)
	bTick &= editor::g_bTickGame;
#endif
	if (bTick)
	{
		tick(dt);
	}
	if (engine::mainWindow()->isClosing())
	{
		engine::mainWindow()->destroy();
	}
}

void World::stopImpl()
{
	m_inputContext.token.reset();
	g_uManifest.reset();
	stop();
	m_registry.clear();
	LOG_I("[{}] stopped", utils::tName(*this));
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
	g_loadedAssets = {};
	g_uManifest.reset();
	s_pActive = nullptr;
	s_worlds.clear();
	s_worldByType.clear();
}

bool World::tick(Time dt, gfx::ScreenRect const& sceneRect)
{
	if (g_pWaitingToStart && !g_uManifest)
	{
		ID previousID;
		bool bSkipUnload = false;
		if (s_pActive)
		{
			previousID = s_pActive->m_id;
			s_pActive->stopImpl();
			bSkipUnload = s_pActive->m_flags.isSet(Flag::eSkipManifestUnload);
			s_pActive = nullptr;
		}
		auto const manifestID = g_pWaitingToStart->manifestID();
		auto toUnload = g_loadedAssets;
		if (!manifestID.empty() && engine::reader().isPresent(manifestID))
		{
			g_uManifest = std::make_unique<AssetManifest>(engine::reader(), manifestID);
			auto const loadList = g_uManifest->parse();
			auto const toLoad = loadList - g_loadedAssets;
			toUnload = g_loadedAssets - loadList;
			g_uManifest->m_toLoad.intersect(toLoad);
		}
		if (bSkipUnload)
		{
			toUnload = {};
		}
		AssetManifest::unload(toUnload);
		g_loadedAssets = g_loadedAssets - toUnload;
		if (g_pWaitingToStart->startImpl(previousID))
		{
			s_pActive = g_pWaitingToStart;
		}
		else
		{
			LOG_E("[{}] Failed to start World{}", utils::tName<World>(), g_pWaitingToStart->id());
		}
		g_pWaitingToStart = nullptr;
	}
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

bool World::isBusy()
{
	return g_uManifest.get() != nullptr;
}

bool World::worldLoadPending()
{
	return g_pWaitingToStart != nullptr;
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
} // namespace le
