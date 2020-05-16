#pragma once
#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include "core/flags.hpp"
#include "core/log_config.hpp"
#include "core/std_types.hpp"
#include "core/time.hpp"
#include "entity.hpp"
#include "component.hpp"

namespace le
{
class Registry
{
public:
	enum class DestroyMode : u8
	{
		eDeferred,	// destroyEntity() sets eDestroyed
		eImmediate, // destroyEntity() erases Entity
		eCOUNT_,
	};

	// Desired flags can be combined with a mask as per-Entity filters for view()
	enum class Flag : u8
	{
		eDisabled,
		eDestroyed,
		eDebug,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	// id => [components]
	template <typename... Comp>
	using View = std::unordered_map<Entity::ID, std::tuple<Comp*...>>;

public:
	static std::string const s_tName;

private:
	// Shared cache of computed signs
	static std::unordered_map<std::type_index, Component::Sign> s_signs;
	// Thread-safe static mutex
	static std::mutex s_mutex;

public:
	// Adjusts log::Level on database changes (unset to disable)
	std::optional<log::Level> m_logLevel = log::Level::eInfo;

protected:
	std::unordered_map<Entity::ID, Flags> m_entityFlags; // Every Entity has a Flags, even if not in m_db
	std::unordered_map<Entity::ID, std::string> m_entityNames;
	std::unordered_map<Component::Sign, std::string> m_componentNames;
	// Database of id => [Sign => [component]]
	std::unordered_map<Entity::ID, std::unordered_map<Component::Sign, std::unique_ptr<Component>>> m_db;
	// Thread-safe member mutex
	mutable std::mutex m_mutex;

private:
	Entity::ID m_nextID = 0;
	DestroyMode m_destroyMode;

public:
	Registry(DestroyMode destroyMode = DestroyMode::eDeferred);
	virtual ~Registry();

public:
	template <typename Comp>
	static Component::Sign signature();

	template <typename... Comps>
	static std::array<Component::Sign, sizeof...(Comps)> signatures();

public:
	template <typename... Comps>
	Entity spawnEntity(std::string name);

	bool setEnabled(Entity entity, bool bEnabled);
	bool setDebug(Entity entity, bool bDebug);
	bool isEnabled(Entity entity) const;
	bool isAlive(Entity entity) const;
	bool isDebugSet(Entity entity) const;

	bool destroyEntity(Entity const& entity);
	bool destroyEntity(Entity& out_entity);

	template <typename Comp, typename... Args>
	Comp* addComponent(Entity entity, Args... args);

	template <typename Comp1, typename Comp2, typename... Comps>
	void addComponent(Entity entity);

	template <typename Comp>
	Comp const* component(Entity entity) const;

	template <typename Comp>
	Comp* component(Entity entity);

	template <typename... Comps>
	bool destroyComponent(Entity entity);

	template <typename Comp1, typename... Comps>
	View<Comp1 const, Comps const...> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {}) const;

	template <typename Comp1, typename... Comps>
	View<Comp1, Comps...> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {});

	void sweep();

	size_t entityCount() const;

private:
	// _Impl functions are not thread safe; they rely on mutexes being locked (and static_asserts being fired)
	// const helpers help with DRY for const and non-const overloads

	template <typename Comp>
	static Component::Sign signature_Impl();

	Entity spawnEntity_Impl(std::string name);

	using EFMap = std::unordered_map<Entity::ID, Flags>;
	EFMap::iterator destroyEntity_Impl(EFMap::iterator iter, Entity::ID id);

	template <typename Comp, typename... Args>
	Component* addComponent_Impl(Entity entity, Args... args);

	template <typename Comp1, typename Comp2, typename... Comps>
	void addComponent_Impl(Entity entity);

	Component* addComponent_Impl(Component::Sign sign, std::unique_ptr<Component>&& uComp, Entity entity);

	template <typename Comp>
	static Comp* component_Impl(std::unordered_map<Component::Sign, std::unique_ptr<Component>>& compMap);

	template <typename Comp>
	static Comp const* component_Impl(std::unordered_map<Component::Sign, std::unique_ptr<Component>> const& compMap);

	void destroyComponent_Impl(Component const* pComponent, Entity::ID id);
	bool destroyComponent_Impl(Entity::ID id);

	// const helper
	template <typename T, typename Comp>
	static Comp* component_Impl(T* pThis, Entity::ID id);

	// const helper
	template <typename T, typename Comp1, typename... Comps>
	static View<Comp1, Comps...> view(T* pThis, Flags mask, Flags pattern);
};

template <typename Comp>
Component::Sign Registry::signature()
{
	static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
	std::scoped_lock<std::mutex> lock(s_mutex);
	return signature_Impl<Comp>();
}

template <typename... Comps>
std::array<Component::Sign, sizeof...(Comps)> Registry::signatures()
{
	static_assert((std::is_base_of_v<Component, Comps> && ...), "Comp must derive from Component!");
	std::scoped_lock<std::mutex> lock(s_mutex);
	return std::array<Component::Sign, sizeof...(Comps)>{signature_Impl<Comps>()...};
}

template <typename... Comps>
Entity Registry::spawnEntity(std::string name)
{
	static_assert((std::is_base_of_v<Component, Comps> && ...), "Comp must derive from Component!");
	std::scoped_lock<std::mutex> lock(m_mutex);
	auto entity = spawnEntity_Impl(std::move(name));
	if constexpr (sizeof...(Comps) > 0)
	{
		addComponent_Impl<Comps...>(entity);
	}
	return entity;
}

template <typename Comp, typename... Args>
Comp* Registry::addComponent(Entity entity, Args... args)
{
	static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
	static_assert((std::is_constructible_v<Comp, Args> && ...), "Cannot construct Comp with given Args...");
	std::scoped_lock<std::mutex> lock(m_mutex);
	if (m_entityFlags.find(entity.id) != m_entityFlags.end())
	{
		return static_cast<Comp*>(addComponent_Impl<Comp>(entity, std::forward<Args>(args)...));
	}
	return nullptr;
}

template <typename Comp1, typename Comp2, typename... Comps>
void Registry::addComponent(Entity entity)
{
	static_assert((std::is_base_of_v<Component, Comps> && ...), "Comp must derive from Component!");
	std::scoped_lock<std::mutex> lock(m_mutex);
	if (m_entityFlags.find(entity.id) != m_entityFlags.end())
	{
		addComponent_Impl<Comp1, Comp2, Comps...>(entity);
	}
	return;
}

template <typename Comp>
Comp const* Registry::component(Entity entity) const
{
	static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
	std::scoped_lock<std::mutex> lock(m_mutex);
	return component_Impl<Registry const, Comp const>(this, entity.id);
}

template <typename Comp>
Comp* Registry::component(Entity entity)
{
	static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
	std::scoped_lock<std::mutex> lock(m_mutex);
	return component_Impl<Registry, Comp>(this, entity.id);
}

template <typename... Comps>
bool Registry::destroyComponent(Entity entity)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	if constexpr (sizeof...(Comps) > 0)
	{
		static_assert((std::is_base_of_v<Component, Comps> && ...), "Comp must derive from Component!");
		if (m_entityFlags.find(entity.id) != m_entityFlags.end())
		{
			auto const signs = signatures<Comps...>();
			for (auto sign : signs)
			{
				if (auto search = m_db[entity.id].find(sign); search != m_db[entity.id].end())
				{
					destroyComponent_Impl(search->second.get(), entity.id);
				}
			}
			return true;
		}
		return false;
	}
	else
	{
		return destroyComponent_Impl(entity.id);
	}
}

template <typename Comp1, typename... Comps>
typename Registry::View<Comp1 const, Comps const...> Registry::view(Flags mask, Flags pattern) const
{
	return view<Registry const, Comp1 const, Comps const...>(this, mask, pattern);
}

template <typename Comp1, typename... Comps>
typename Registry::View<Comp1, Comps...> Registry::view(Flags mask, Flags pattern)
{
	return view<Registry, Comp1, Comps...>(this, mask, pattern);
}

template <typename Comp>
Component::Sign Registry::signature_Impl()
{
	auto const& t = typeid(Comp);
	auto const index = std::type_index(t);
	auto search = s_signs.find(index);
	if (search == s_signs.end())
	{
		auto result = s_signs.emplace(index, (Component::Sign)t.hash_code());
		ASSERT(result.second, "Insertion failure");
		search = result.first;
	}
	return search->second;
}

template <typename Comp, typename... Args>
Component* Registry::addComponent_Impl(Entity entity, Args... args)
{
	return addComponent_Impl(signature<Comp>(), std::make_unique<Comp>(std::forward<Args>(args)...), entity);
}

template <typename Comp1, typename Comp2, typename... Comps>
void Registry::addComponent_Impl(Entity entity)
{
	addComponent_Impl<Comp1>(entity);
	addComponent_Impl<Comp2, Comps...>(entity);
}

template <typename Comp>
Comp* Registry::component_Impl(std::unordered_map<Component::Sign, std::unique_ptr<Component>>& compMap)
{
	auto const sign = signature<Comp>();
	if (auto search = compMap.find(sign); search != compMap.end())
	{
		return static_cast<Comp*>(search->second.get());
	}
	return nullptr;
}

template <typename Comp>
Comp const* Registry::component_Impl(std::unordered_map<Component::Sign, std::unique_ptr<Component>> const& compMap)
{
	auto const sign = signature<Comp>();
	if (auto search = compMap.find(sign); search != compMap.end())
	{
		return static_cast<Comp const*>(search->second.get());
	}
	return nullptr;
}

template <typename T, typename Comp>
Comp* Registry::component_Impl(T* pThis, Entity::ID id)
{
	auto cSearch = pThis->m_db.find(id);
	if (cSearch != pThis->m_db.end())
	{
		return component_Impl<Comp>(cSearch->second);
	}
	return nullptr;
}

template <typename T, typename Comp1, typename... Comps>
typename Registry::View<Comp1, Comps...> Registry::view(T* pThis, Flags mask, Flags pattern)
{
	static_assert(std::is_base_of_v<Component, Comp1> && (std::is_base_of_v<Component, Comps> && ...), "Comp must derive from Component!");
	View<Comp1, Comps...> ret;
	static auto const signs = signatures<Comp1, Comps...>();
	std::scoped_lock<std::mutex> lock(pThis->m_mutex);
	for (auto const [id, flags] : pThis->m_entityFlags)
	{
		if ((flags & mask) == (pattern & mask))
		{
			auto search = pThis->m_db.find(id);
			if (search != pThis->m_db.end())
			{
				auto& compMap = search->second;
				auto checkSigns = [&compMap](auto sign) -> bool { return compMap.find(sign) != compMap.end(); };
				bool const bHasAll = std::all_of(signs.begin(), signs.end(), checkSigns);
				if (bHasAll)
				{
					ret[id] = std::make_tuple(component_Impl<Comp1>(compMap), (component_Impl<Comps>(compMap))...);
				}
			}
		}
	}
	return ret;
}
} // namespace le
