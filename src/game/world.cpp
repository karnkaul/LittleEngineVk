#include <core/log.hpp>
#include <engine/game/world.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/levk.hpp>
#include <editor/editor.hpp>
#include <game/input_impl.hpp>
#include <resources/manifest.hpp>
#include <levk_impl.hpp>

namespace le
{
namespace
{
struct
{
	std::unique_ptr<AssetManifest> uManifest;
	AssetList loadedAssets;
	World* pNext = nullptr;
} g_data;
} // namespace

std::unordered_map<World::ID, std::unique_ptr<World>> World::s_worlds;
std::unordered_map<std::type_index, World*> World::s_worldByType;
World* World::s_pActive = nullptr;
World::ID World::s_lastID;

World::World() = default;
World::~World()
{
	LOG_I("[{}] World{} Destroyed", m_name.empty() ? "UnknownWorld" : m_name, m_id);
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

bool World::isBusy()
{
	return g_data.uManifest.get() != nullptr;
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

bool World::startImpl(ID previous)
{
	m_previousWorldID = previous;
	m_inputContext = {};
#if defined(LEVK_DEBUG)
	m_inputContext.context.m_name = m_name;
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

void World::tickImpl(Time dt)
{
	m_registry.flush();
	bool bTick = true;
	if (g_data.uManifest)
	{
		auto const status = g_data.uManifest->update();
		switch (status)
		{
		case AssetManifest::Status::eIdle:
		{
			onManifestLoaded();
			g_data.loadedAssets = std::move(g_data.uManifest->m_loaded);
			g_data.uManifest.reset();
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
}

void World::stopImpl()
{
	m_inputContext.token.reset();
	g_data.uManifest.reset();
	stop();
	m_registry.clear();
	LOG_I("[{}] stopped", utils::tName(*this));
}

bool World::start(ID id)
{
	input::g_bFire = true;
	if (auto search = s_worlds.find(id); search != s_worlds.end())
	{
		auto const& uWorld = search->second;
		auto const manifestID = uWorld->manifestID();
		if (engine::reader().isPresent(manifestID))
		{
			g_data.uManifest = std::make_unique<AssetManifest>(engine::reader(), manifestID);
		}
		if (uWorld->startImpl())
		{
			if (g_data.uManifest)
			{
				g_data.uManifest->start();
			}
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

void World::startNext()
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
	auto const manifestID = g_data.pNext->manifestID();
	auto toUnload = g_data.loadedAssets;
	if (!manifestID.empty() && engine::reader().isPresent(manifestID))
	{
		g_data.uManifest = std::make_unique<AssetManifest>(engine::reader(), manifestID);
		auto const loadList = g_data.uManifest->parse();
		auto const toLoad = loadList - g_data.loadedAssets;
		toUnload = g_data.loadedAssets - loadList;
		g_data.uManifest->m_toLoad.intersect(toLoad);
	}
	if (bSkipUnload)
	{
		toUnload = {};
	}
	AssetManifest::unload(toUnload);
	g_data.loadedAssets = g_data.loadedAssets - toUnload;
	if (g_data.pNext->startImpl(previousID))
	{
		s_pActive = g_data.pNext;
		if (g_data.uManifest)
		{
			g_data.uManifest->start();
		}
	}
	else
	{
		LOG_E("[{}] Failed to start World{}", utils::tName<World>(), g_data.pNext->id());
	}
	g_data.pNext = nullptr;
}

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
	stopActive();
	g_data = {};
	s_pActive = nullptr;
	s_worlds.clear();
	s_worldByType.clear();
}

bool World::tick(Time dt, gfx::ScreenRect const& sceneRect)
{
	if (!engine::mainWindow())
	{
		return false;
	}
	if (g_data.pNext && !g_data.uManifest)
	{
		startNext();
	}
	if (s_pActive)
	{
		s_pActive->m_worldRect = sceneRect;
		s_pActive->tickImpl(dt);
		return true;
	}
	return false;
}

bool World::submitScene(gfx::Renderer& out_renderer, gfx::Camera const& camera)
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
