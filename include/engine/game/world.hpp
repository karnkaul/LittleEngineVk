#pragma once
#include <filesystem>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <core/assert.hpp>
#include <core/std_types.hpp>
#include <core/utils.hpp>
#include <core/time.hpp>
#include <core/zero.hpp>
#include <engine/ecs/registry.hpp>
#include <engine/gfx/renderer.hpp>
#include <core/transform.hpp>

namespace le
{
namespace stdfs = std::filesystem;

namespace engine
{
class Service;
}

class World
{
public:
	using ID = TZero<s32, -1>;

protected:
	Registry m_registry = Registry(Registry::DestroyMode::eDeferred);
	std::string m_tName;

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
	static std::unordered_map<s32, std::unique_ptr<World>> s_worlds;
	static std::unordered_map<std::type_index, World*> s_worldByType;
	static World* s_pActive;
	static ID s_lastID;

public:
	template <typename T, typename... Args>
	static ID addWorld(Args... args);

	template <typename T>
	static T* getWorld();

	template <typename T>
	static bool removeWorld();

	static World* getWorld(ID id);
	static bool removeWorld(ID id);

	static bool loadWorld(ID id);

	static World* active();

public:
	World();
	virtual ~World();

public:
	ID id() const;
	std::string_view name() const;
	bool switchWorld(ID newWorld);

	Entity spawnEntity(std::string name, bool bAddTransform = true);
	bool destroyEntity(Entity entity);

protected:
	virtual bool start() = 0;
	virtual void tick(Time dt) = 0;
	virtual gfx::Renderer::Scene buildScene() const = 0;
	virtual void stop() = 0;

protected:
	virtual stdfs::path manifestID() const;
	virtual void onManifestLoaded();

protected:
	class Window* window() const;

private:
	bool startImpl();
	void tickImpl(Time dt);
	void stopImpl();

#if defined(LEVK_EDITOR)
public:
	Registry& registry();
#endif

private:
	static bool stopActive();
	static void destroyAll();
	static bool tick(Time dt, gfx::ScreenRect const& sceneRect);
	static bool submitScene(gfx::Renderer& renderer);
	// TODO: private
public:
	static bool start(ID id);

private:
	friend class engine::Service;
};

template <typename T, typename... Args>
World::ID World::addWorld(Args... args)
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
			uT->m_tName = utils::tName<T>();
			uT->m_id = ++s_lastID.handle;
			uT->m_type = type;
			s_worldByType.emplace(type, uT.get());
			s_worlds.emplace(s_lastID, std::move(uT));
		}
	}
	return s_lastID;
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
} // namespace le
