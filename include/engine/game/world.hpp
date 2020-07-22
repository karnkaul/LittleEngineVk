#pragma once
#include <filesystem>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <core/assert.hpp>
#include <core/flags.hpp>
#include <core/std_types.hpp>
#include <core/utils.hpp>
#include <core/time.hpp>
#include <core/transform.hpp>
#include <core/zero.hpp>
#include <engine/ecs/registry.hpp>
#include <engine/game/input.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/gfx/renderer.hpp>

namespace le
{
namespace stdfs = std::filesystem;

namespace engine
{
class Service;
} // namespace engine

class SceneBuilder;

class World
{
public:
	using ID = TZero<s32, -1>;

protected:
	enum class Flag : s8
	{
		eSkipManifestUnload,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	Flags m_flags;

protected:
	Registry m_registry = Registry(Registry::DestroyMode::eDeferred);
	std::string m_name;
	input::CtxWrapper m_inputContext;
	std::unique_ptr<gfx::Camera> m_uSceneCam;
	ID m_previousWorldID;

#if defined(LEVK_EDITOR)
public:
	using EMap = std::unordered_map<Transform*, Entity>;
	Transform m_root;
	EMap m_transformToEntity;
#endif

private:
	gfx::ScreenRect m_worldRect;
	ID m_id;
	std::type_index m_type = std::type_index(typeid(World));

private:
	static std::unordered_map<ID, std::unique_ptr<World>> s_worlds;
	static std::unordered_map<std::type_index, World*> s_worldByType;
	static World* s_pActive;
	static ID s_lastID;

public:
	template <typename T, typename... Args>
	static ID addWorld(Args&&... args);

	template <typename T1, typename T2, typename... Tn>
	static void addWorld();

	template <typename T>
	static T* getWorld();

	template <typename T>
	static bool removeWorld();

	static World* getWorld(ID id);
	static bool removeWorld(ID id);

	static bool loadWorld(ID id);
	static World* active();

	static bool isBusy();
	static bool worldLoadPending();

#if defined(LEVK_EDITOR)
	static std::vector<World*> allWorlds();
#endif

public:
	World();
	virtual ~World();

public:
	ID id() const;
	std::string_view name() const;
	bool switchWorld(ID newWorld);

	Entity spawnEntity(std::string name, bool bAddTransform = true);
	bool destroyEntity(Entity entity);

	template <typename T = gfx::Camera, typename... Args>
	T& setSceneCamera(Args&&... args);

	template <typename T = gfx::Camera>
	T& sceneCamera();

	template <typename T = gfx::Camera>
	T const* sceneCamPtr() const;

protected:
	virtual bool start() = 0;
	virtual void tick(Time dt) = 0;
	virtual SceneBuilder const& sceneBuilder() const = 0;
	virtual void stop() = 0;

protected:
	virtual stdfs::path manifestID() const;
	virtual stdfs::path inputMapID() const;
	virtual void onManifestLoaded();

#if defined(LEVK_EDITOR)
public:
	Registry& registry();
#endif

private:
	bool startImpl(ID previous = {});
	void tickImpl(Time dt);
	void stopImpl();

private:
	static bool start(ID id);
	static void startNext();
	static bool stopActive();
	static void destroyAll();
	static bool tick(Time dt, gfx::ScreenRect const& sceneRect);
	static bool submitScene(gfx::Renderer& out_renderer, gfx::Camera const& camera);

	template <typename T, typename Th>
	static T* sceneCamPtr(Th* pThis);

private:
	friend class engine::Service;
};

template <typename T, typename... Args>
World::ID World::addWorld(Args&&... args)
{
	static_assert(std::is_base_of_v<World, T>, "T must derive from World!");
	auto const type = std::type_index(typeid(T));
	bool const bExists = s_worldByType.find(type) != s_worldByType.end();
	ASSERT(!bExists, "World Type already added!");
	if (!bExists)
	{
		auto uT = std::make_unique<T>(std::forward<Args>(args)...);
		if (uT)
		{
			if (uT->m_name.empty())
			{
				uT->m_name = utils::tName<T>();
			}
			uT->m_id = ++s_lastID.payload;
			uT->m_type = type;
			s_worldByType.emplace(type, uT.get());
			s_worlds.emplace(s_lastID, std::move(uT));
		}
	}
	return s_lastID;
}

template <typename T1, typename T2, typename... Tn>
void World::addWorld()
{
	addWorld<T1>();
	addWorld<T2, Tn...>();
}

template <typename T>
T* World::getWorld()
{
	auto search = s_worldByType.find(std::type_index(typeid(T)));
	return search != s_worldByType.end() ? dynamic_cast<T*>(search->second) : nullptr;
}

template <typename T>
bool World::removeWorld()
{
	auto search = s_worldByType.find(std::type_index(typeid(T)));
	return search != s_worldByType.end() ? removeWorld(search->second->m_id) : false;
}

template <typename T, typename... Args>
T& World::setSceneCamera(Args&&... args)
{
	m_uSceneCam = std::make_unique<T>(std::forward<Args>(args)...);
	return *sceneCamPtr<T, World>(this);
}

template <typename T>
T& World::sceneCamera()
{
	if (!m_uSceneCam)
	{
		return setSceneCamera<T>();
	}
	return *sceneCamPtr<T, World>(this);
}

template <typename T>
T const* World::sceneCamPtr() const
{
	return sceneCamPtr<T const, World const>(this);
}

template <typename T, typename Th>
T* World::sceneCamPtr(Th* pThis)
{
	static_assert(std::is_base_of_v<gfx::Camera, T>, "T must derive from Camera!");
	if constexpr (std::is_same_v<T, gfx::Camera>)
	{
		return pThis->m_uSceneCam.get();
	}
	else
	{
		return dynamic_cast<T*>(pThis->m_uSceneCam.get());
	}
}
} // namespace le
