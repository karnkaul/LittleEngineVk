#pragma once
#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "core/std_types.hpp"
#include "core/time.hpp"
#include "entity.hpp"
#include "component.hpp"

namespace le::ecs
{
struct CompQuery final
{
	std::unordered_map<Component::Sign, Component*> results;

	template <typename Comp>
	Comp const* get() const;

	template <typename Comp>
	Comp* get();

	template <typename T, typename Comp>
	static Comp* get(T* pThis);
};

class Registry
{
public:
	enum class DestroyMode : u8
	{
		eDeferred,
		eImmediate,
		eCOUNT_,
	};

protected:
	enum class EntityFlag : u8
	{
		eDisabled,
		eDestroyed,
		eCOUNT_
	};
	using EntityFlags = TFlags<EntityFlag>;

protected:
	std::unordered_map<Entity::ID, Entity> m_entities;
	std::unordered_map<Entity::ID, EntityFlags> m_entityFlags;
	std::unordered_map<Entity::ID, std::string> m_entityNames;
	std::unordered_map<Component::Sign, std::string> m_componentNames;
	std::unordered_map<Entity::ID, std::unordered_map<Component::Sign, std::unique_ptr<Component>>> m_db;

private:
	Entity::ID m_nextID = Entity::s_invalidID;
	DestroyMode m_destroyMode;

public:
	static std::string const s_tName;

public:
	Registry(DestroyMode destroyMode = DestroyMode::eDeferred);
	Registry(Registry&&);
	Registry& operator=(Registry&&);
	virtual ~Registry();

public:
	template <typename T>
	static Component::Sign signature()
	{
		return (Component::Sign) typeid(T).hash_code();
	}

	template <typename... Comps>
	constexpr static size_t count()
	{
		static_assert((std::is_base_of_v<Component, Comps> && ...), "Comp must derive from Component!");
		return sizeof...(Comps);
	}

	template <typename Comp1, typename... Comps>
	static std::vector<Component::Sign> getSigns()
	{
		std::vector<Component::Sign> ret;
		ret.reserve(count<Comp1, Comps...>());
		ret.push_back(signature<Comp1>());
		(ret.push_back(signature<Comps>()), ...);
		return ret;
	}

public:
	template <typename Comp1, typename... Comps>
	Entity::ID spawnEntity(std::string name)
	{
		auto eEntity = spawnEntity(name);
		if (auto pEntity = entity(eEntity))
		{
			addComponent_Impl<Comp1, Comps...>(pEntity);
			return pEntity->m_id;
		}
		return Entity::s_invalidID;
	}

	Entity::ID spawnEntity(std::string name);
	Entity* entity(Entity::ID id);
	Entity const* entity(Entity::ID id) const;
	bool destroyEntity(Entity::ID id);
	bool destroyComponents(Entity::ID id);
	bool setEnabled(Entity::ID id, bool bEnabled);

	bool isEnabled(Entity::ID id) const;
	bool isAlive(Entity::ID id) const;

	template <typename Comp, typename... Args>
	Comp* addComponent(Entity::ID id, Args... args)
	{
		if (auto pEntity = entity(id))
		{
			return dynamic_cast<Comp*>(addComponent_Impl<Comp>(pEntity, std::forward<Args>(args)...));
		}
		return nullptr;
	}

	template <typename Comp1, typename Comp2, typename... Comps>
	void addComponent(Entity::ID id)
	{
		if (auto pEntity = entity(id))
		{
			addComponent_Impl<Comp1, Comp2, Comps...>(pEntity);
		}
		return;
	}

	template <typename Comp>
	Comp const* component(Entity::ID id) const
	{
		return component<Registry const, Comp const>(this, id);
	}

	template <typename Comp>
	Comp* component(Entity::ID id)
	{
		return component<Registry, Comp>(this, id);
	}

	template <typename Comp>
	bool destroyComponent(Entity::ID id)
	{
		static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
		if (entity(id))
		{
			auto const sign = signature<Comp>();
			auto search = m_db[id].find(sign);
			if (search != m_db[id].end())
			{
				detach(search->second.get(), id);
				return true;
			}
		}
		return false;
	}

	template <typename Comp1, typename... Comps>
	std::unordered_map<Entity::ID, CompQuery> view(bool bOnlyEnabled = true) const
	{
		return view<Registry const, Comp1, Comps...>(this, bOnlyEnabled);
	}

	template <typename Comp1, typename... Comps>
	std::unordered_map<Entity::ID, CompQuery> view(bool bOnlyEnabled = true)
	{
		return view<Registry, Comp1, Comps...>(this, bOnlyEnabled);
	}

public:
	void cleanDestroyed();

private:
	template <typename Comp, typename... Args>
	Component* addComponent_Impl(Entity* pEntity, Args... args)
	{
		static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
		static_assert((std::is_constructible_v<Comp, Args> && ...), "Cannot construct Comp with given Args...");
		return attach(signature<Comp>(), std::make_unique<Comp>(std::forward<Args>(args)...), pEntity);
	}

	template <typename Comp1, typename Comp2, typename... Comps>
	void addComponent_Impl(Entity* pEntity)
	{
		addComponent_Impl<Comp1>(pEntity);
		addComponent_Impl<Comp2, Comps...>(pEntity);
	}

	// const helper
	template <typename T, typename Comp>
	static Comp* component(T* pThis, Entity::ID id)
	{
		static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
		auto const sign = signature<Comp>();
		auto cSearch = pThis->m_db.find(id);
		if (cSearch != pThis->m_db.end())
		{
			auto& cmap = cSearch->second;
			auto search = cmap.find(sign);
			if (search != cmap.end())
			{
				return dynamic_cast<Comp*>(search->second.get());
			}
		}
		return nullptr;
	}

	// const helper
	template <typename T, typename Comp1, typename... Comps>
	static std::unordered_map<Entity::ID, CompQuery> view(T* pThis, bool bOnlyEnabled)
	{
		std::unordered_map<Entity::ID, CompQuery> ret;
		auto const signs = getSigns<Comp1, Comps...>();
		for (auto& [id, _] : pThis->m_entities)
		{
			auto flags = pThis->m_entityFlags[id];
			if (!flags.isSet(EntityFlag::eDestroyed) && (!bOnlyEnabled || !flags.isSet(EntityFlag::eDisabled)))
			{
				auto& components = pThis->m_db[id];
				bool const bHasAll = std::all_of(signs.begin(), signs.end(),
												 [&components](auto sign) -> bool { return components.find(sign) != components.end(); });
				if (bHasAll)
				{
					auto& ref = ret[id];
					for (auto sign : signs)
					{
						ref.results[sign] = components[sign].get();
						ASSERT(ref.results[sign] != nullptr, "Invariant violated");
					}
				}
			}
		}
		return ret;
	}

	Component* attach(Component::Sign sign, std::unique_ptr<Component>&& uComp, Entity* pEntity);
	void detach(Component const* pComponent, Entity::ID id);

	using EntityMap = std::unordered_map<Entity::ID, Entity>;
	EntityMap::iterator destroyEntity(EntityMap::iterator iter, Entity::ID id);
};

template <typename Comp>
Comp const* CompQuery::get() const
{
	return get<CompQuery const, Comp const>(this);
}

template <typename Comp>
Comp* CompQuery::get()
{
	return get<CompQuery, Comp>(this);
}

template <typename T, typename Comp>
Comp* CompQuery::get(T* pThis)
{
	static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
	auto const sign = Registry::signature<Comp>();
	auto const search = pThis->results.find(sign);
	return search != pThis->results.end() ? dynamic_cast<Comp*>(search->second) : nullptr;
}
} // namespace le::ecs
