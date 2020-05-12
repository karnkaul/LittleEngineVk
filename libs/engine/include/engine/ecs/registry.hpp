#pragma once
#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "core/flags.hpp"
#include "core/std_types.hpp"
#include "core/time.hpp"
#include "entity.hpp"
#include "component.hpp"

namespace le
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

	enum class Flag : u8
	{
		eDisabled,
		eDestroyed,
		eDebug,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

protected:
	std::unordered_map<Entity::ID, Flags> m_entityFlags;
	std::unordered_map<Entity::ID, std::string> m_entityNames;
	std::unordered_map<Component::Sign, std::string> m_componentNames;
	std::unordered_map<Entity::ID, std::unordered_map<Component::Sign, std::unique_ptr<Component>>> m_db;
	mutable std::mutex m_mutex;

private:
	Entity::ID m_nextID = 0;
	DestroyMode m_destroyMode;

public:
	static std::string const s_tName;

public:
	Registry(DestroyMode destroyMode = DestroyMode::eDeferred);
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

public:
	Entity spawnEntity(std::string name);

	template <typename Comp1, typename... Comps>
	Entity spawnEntity(std::string name)
	{
		auto entity = spawnEntity(std::move(name));
		addComponent_Impl<Comp1, Comps...>(entity);
	}

	bool destroyEntity(Entity entity);
	bool destroyComponents(Entity entity);
	bool setEnabled(Entity entity, bool bEnabled);
	bool setDebug(Entity entity, bool bDebug);

	bool isEnabled(Entity entity) const;
	bool isAlive(Entity entity) const;
	bool isDebugSet(Entity entity) const;

	template <typename Comp, typename... Args>
	Comp* addComponent(Entity entity, Args... args)
	{
		if (m_entityFlags.find(entity.id) != m_entityFlags.end())
		{
			return dynamic_cast<Comp*>(addComponent_Impl<Comp>(entity, std::forward<Args>(args)...));
		}
		return nullptr;
	}

	template <typename Comp1, typename Comp2, typename... Comps>
	void addComponent(Entity entity)
	{
		if (m_entityFlags.find(entity.id) != m_entityFlags.end())
		{
			addComponent_Impl<Comp1, Comp2, Comps...>(entity);
		}
		return;
	}

	template <typename Comp>
	Comp const* component(Entity entity) const
	{
		return component<Registry const, Comp const>(this, entity.id);
	}

	template <typename Comp>
	Comp* component(Entity entity)
	{
		return component<Registry, Comp>(this, entity.id);
	}

	template <typename Comp>
	bool destroyComponent(Entity entity)
	{
		static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
		if (m_entityFlags.find(entity.id) != m_entityFlags.end())
		{
			auto const sign = signature<Comp>();
			if (auto search = m_db[entity.id].find(sign); search != m_db[entity.id].end())
			{
				detach(search->second.get(), entity);
				return true;
			}
		}
		return false;
	}

	template <typename Comp1, typename... Comps>
	std::unordered_map<Entity::ID, CompQuery> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {}) const
	{
		return view<Registry const, Comp1, Comps...>(this, mask, pattern);
	}

	template <typename Comp1, typename... Comps>
	std::unordered_map<Entity::ID, CompQuery> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {})
	{
		return view<Registry, Comp1, Comps...>(this, mask, pattern);
	}

public:
	void cleanDestroyed();

private:
	template <typename Comp, typename... Args>
	Component* addComponent_Impl(Entity entity, Args... args)
	{
		static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
		static_assert((std::is_constructible_v<Comp, Args> && ...), "Cannot construct Comp with given Args...");
		return attach(signature<Comp>(), std::make_unique<Comp>(std::forward<Args>(args)...), entity);
	}

	template <typename Comp1, typename Comp2, typename... Comps>
	void addComponent_Impl(Entity entity)
	{
		addComponent_Impl<Comp1>(entity);
		addComponent_Impl<Comp2, Comps...>(entity);
	}

	// const helper
	template <typename T, typename Comp>
	static Comp* component(T* pThis, Entity::ID id)
	{
		static_assert(std::is_base_of_v<Component, Comp>, "Comp must derive from Component!");
		auto const sign = signature<Comp>();
		std::unique_lock<std::mutex> lock(pThis->m_mutex);
		auto cSearch = pThis->m_db.find(id);
		if (cSearch != pThis->m_db.end())
		{
			auto& cmap = cSearch->second;
			if (auto search = cmap.find(sign); search != cmap.end())
			{
				return dynamic_cast<Comp*>(search->second.get());
			}
		}
		return nullptr;
	}

	// const helper
	template <typename T, typename Comp1, typename... Comps>
	static std::unordered_map<Entity::ID, CompQuery> view(T* pThis, Flags mask, Flags pattern)
	{
		std::unordered_map<Entity::ID, CompQuery> ret;
		auto const signs = {signature<Comp1>(), (signature<Comps>(), ...)};
		std::unique_lock<std::mutex> lock(pThis->m_mutex);
		for (auto& [id, flags] : pThis->m_entityFlags)
		{
			if ((flags & mask) == (pattern & mask))
			{
				auto& comps = pThis->m_db[id];
				auto checkSigns = [&comps](auto sign) -> bool { return comps.find(sign) != comps.end(); };
				bool const bHasAll = std::all_of(signs.begin(), signs.end(), checkSigns);
				if (bHasAll)
				{
					auto& ref = ret[id];
					for (auto sign : signs)
					{
						ref.results[sign] = comps[sign].get();
						ASSERT(ref.results[sign] != nullptr, "Invariant violated");
					}
				}
			}
		}
		return ret;
	}

	Component* attach(Component::Sign sign, std::unique_ptr<Component>&& uComp, Entity entity);
	void detach(Component const* pComponent, Entity::ID id);

	using EFMap = std::unordered_map<Entity::ID, Flags>;
	EFMap::iterator destroyEntity(EFMap::iterator iter, Entity::ID id);
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
} // namespace le
