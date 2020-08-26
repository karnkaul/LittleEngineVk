#pragma once
#include <filesystem>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <core/assert.hpp>
#include <core/atomic_counter.hpp>
#include <core/flags.hpp>
#include <core/std_types.hpp>
#include <core/utils.hpp>
#include <core/time.hpp>
#include <core/transform.hpp>
#include <core/zero.hpp>
#include <core/ecs_registry.hpp>
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
	using Semaphore = Counter<s32>::Semaphore;

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
	input::Context m_inputContext;
	Token m_inputToken;
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
	inline static std::unordered_map<ID, std::unique_ptr<World>> s_worlds;
	inline static std::unordered_map<std::type_index, Ref<World>> s_worldByType;
	inline static Counter<s32> s_busyCounter;
	inline static World* s_pActive = nullptr;
	inline static ID s_lastID;

public:
	template <typename T, typename... Args>
	static ID addWorld(Args&&... args);

	template <typename T1, typename T2, typename... Tn>
	static void addWorld();

	template <typename T>
	static T* world();

	template <typename T>
	static bool removeWorld();

	static World* world(ID id);
	static bool removeWorld(ID id);

	static bool loadWorld(ID id);
	static World* active();

	static Semaphore setBusy();
	static bool isBusy();
	static bool worldLoadPending();

#if defined(LEVK_EDITOR)
	static std::vector<Ref<World>> allWorlds();
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
	bool impl_start(ID previous = {});
	void impl_tick(gfx::ScreenRect const& worldRect, Time dt, bool bTickSelf);
	void impl_stop();

private:
	static bool impl_start(World& out_world, ID prev);
	static bool impl_startID(ID id);
	static void impl_startNext();
	static bool impl_stopActive();
	static void impl_destroyAll();
	static bool impl_tick(Time dt, gfx::ScreenRect const& sceneRect, bool bTickActive, bool bTerminate);
	static bool impl_submitScene(gfx::Renderer& out_renderer, gfx::Camera const& camera);

	template <typename T, typename Th>
	static T* impl_sceneCamPtr(Th* pThis);

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
			s_worldByType.emplace(type, *uT);
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
T* World::world()
{
	auto search = s_worldByType.find(std::type_index(typeid(T)));
	return search != s_worldByType.end() ? dynamic_cast<T*>(&(World&)search->second) : nullptr;
}

template <typename T>
bool World::removeWorld()
{
	auto search = s_worldByType.find(std::type_index(typeid(T)));
	return search != s_worldByType.end() ? removeWorld(((World&)search->second).m_id) : false;
}

template <typename T, typename... Args>
T& World::setSceneCamera(Args&&... args)
{
	m_uSceneCam = std::make_unique<T>(std::forward<Args>(args)...);
	return *impl_sceneCamPtr<T, World>(this);
}

template <typename T>
T& World::sceneCamera()
{
	if (!m_uSceneCam)
	{
		return setSceneCamera<T>();
	}
	return *impl_sceneCamPtr<T, World>(this);
}

template <typename T>
T const* World::sceneCamPtr() const
{
	return impl_sceneCamPtr<T const, World const>(this);
}

template <typename T, typename Th>
T* World::impl_sceneCamPtr(Th* pThis)
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
