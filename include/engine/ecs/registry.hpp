#pragma once
#include <algorithm>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <core/flags.hpp>
#include <core/log_config.hpp>
#include <core/std_types.hpp>

namespace le
{
// Entity is a glorified, type-safe ID
struct Entity final
{
	using ID = u64;
	ID id = 0;
};

struct EntityHasher final
{
	size_t operator()(Entity const& entity) const;
};

bool operator==(Entity lhs, Entity rhs);
bool operator!=(Entity lhs, Entity rhs);

class Registry
{
public:
	enum class DestroyMode : s8
	{
		eDeferred,	// destroyEntity() sets eDestroyed
		eImmediate, // destroyEntity() erases Entity
		eCOUNT_,
	};

	// Desired flags can be combined with a mask as per-Entity filters for view()
	enum class Flag : s8
	{
		eDisabled,
		eDestroyed,
		eDebug,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	// Hash code of component type
	using Signature = size_t;

	template <typename... T>
	using View = std::unordered_map<Entity, std::tuple<T*...>, EntityHasher>;

private:
	// Concept
	struct Component
	{
		Signature sign = 0;
		virtual ~Component();
	};

	template <typename T>
	struct Model final : Component
	{
		T t;

		template <typename... Args>
		Model(Args&&... args);
	};

public:
	static std::string const s_tName;

private:
	// Shared cache of computed signs
	static std::unordered_map<std::type_index, Signature> s_signs;
	// Thread-safe static mutex
	static std::mutex s_mutex;

public:
	// Adjusts log::Level on database changes (unset to disable)
	std::optional<log::Level> m_logLevel = log::Level::eInfo;

protected:
	std::unordered_map<Entity::ID, Flags> m_entityFlags; // Every Entity has a Flags, even if not in m_db
	std::unordered_map<Entity::ID, std::string> m_entityNames;
	std::unordered_map<Signature, std::string> m_componentNames;
	// Database of id => [Sign => [component]]
	std::unordered_map<Entity::ID, std::unordered_map<Signature, std::unique_ptr<Component>>> m_db;
	// Thread-safe member mutex
	mutable std::mutex m_mutex;

private:
	Entity::ID m_nextID = 0;
	DestroyMode m_destroyMode;

public:
	Registry(DestroyMode destroyMode = DestroyMode::eDeferred);
	virtual ~Registry();

public:
	template <typename T>
	static Signature signature();

	template <typename... Ts>
	static std::array<Signature, sizeof...(Ts)> signatures();

public:
	template <typename... Ts>
	Entity spawnEntity(std::string name);

	bool setEnabled(Entity entity, bool bEnabled);
	bool setDebug(Entity entity, bool bDebug);
	bool isEnabled(Entity entity) const;
	bool isAlive(Entity entity) const;
	bool isDebugSet(Entity entity) const;

	bool destroyEntity(Entity const& entity);
	bool destroyEntity(Entity& out_entity);

	template <typename T, typename... Args>
	T* addComponent(Entity entity, Args... args);

	template <typename T1, typename T2, typename... Ts>
	void addComponent(Entity entity);

	template <typename T>
	T const* component(Entity entity) const;

	template <typename T>
	T* component(Entity entity);

	template <typename... Ts>
	bool destroyComponent(Entity entity);

	template <typename T1, typename... Ts>
	View<T1 const, Ts const...> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {}) const;

	template <typename T1, typename... Ts>
	View<T1, Ts...> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {});

	void flush();
	void clear();

	size_t entityCount() const;
	std::string_view entityName(Entity entity) const;

private:
	// _Impl functions are not thread safe; they rely on mutexes being locked
	// const helpers help with DRY for const and non-const overloads

	template <typename T>
	static Signature signature_Impl();

	Entity spawnEntity_Impl(std::string name);

	using EFMap = std::unordered_map<Entity::ID, Flags>;
	EFMap::iterator destroyEntity_Impl(EFMap::iterator iter, Entity::ID id);

	template <typename T, typename... Args>
	Component* addComponent_Impl(Entity entity, Args... args);

	template <typename T1, typename T2, typename... Ts>
	void addComponent_Impl(Entity entity);

	Component* addComponent_Impl(Signature sign, std::unique_ptr<Component>&& uT, Entity entity);

	template <typename T>
	static T* component_Impl(std::unordered_map<Signature, std::unique_ptr<Component>>& compMap);

	template <typename T>
	static T const* component_Impl(std::unordered_map<Signature, std::unique_ptr<Component>> const& compMap);

	void destroyComponent_Impl(Component const* pComponent, Entity::ID id);
	bool destroyComponent_Impl(Entity::ID id);

	// const helper
	template <typename Th, typename T>
	static T* component_Impl(Th* pThis, Entity::ID id);

	// const helper
	template <typename Th, typename T1, typename... Ts>
	static View<T1, Ts...> view(Th* pThis, Flags mask, Flags pattern);
};

template <typename T>
template <typename... Args>
Registry::Model<T>::Model(Args&&... args) : t(std::forward<Args>(args)...)
{
}

template <typename T>
Registry::Signature Registry::signature()
{
	std::scoped_lock<std::mutex> lock(s_mutex);
	return signature_Impl<T>();
}

template <typename... Ts>
std::array<Registry::Signature, sizeof...(Ts)> Registry::signatures()
{
	std::scoped_lock<std::mutex> lock(s_mutex);
	return std::array<Signature, sizeof...(Ts)>{signature_Impl<Ts>()...};
}

template <typename... Ts>
Entity Registry::spawnEntity(std::string name)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	auto entity = spawnEntity_Impl(std::move(name));
	if constexpr (sizeof...(Ts) > 0)
	{
		addComponent_Impl<Ts...>(entity);
	}
	return entity;
}

template <typename T, typename... Args>
T* Registry::addComponent(Entity entity, Args... args)
{
	static_assert((std::is_constructible_v<T, Args> && ...), "Cannot construct Comp with given Args...");
	std::scoped_lock<std::mutex> lock(m_mutex);
	if (m_entityFlags.find(entity.id) != m_entityFlags.end())
	{
		return &static_cast<Model<T>*>(addComponent_Impl<T>(entity, std::forward<Args>(args)...))->t;
	}
	return nullptr;
}

template <typename T1, typename T2, typename... Ts>
void Registry::addComponent(Entity entity)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	if (m_entityFlags.find(entity.id) != m_entityFlags.end())
	{
		addComponent_Impl<T1, T2, Ts...>(entity);
	}
	return;
}

template <typename T>
T const* Registry::component(Entity entity) const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	return component_Impl<Registry const, T const>(this, entity.id);
}

template <typename T>
T* Registry::component(Entity entity)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	return component_Impl<Registry, T>(this, entity.id);
}

template <typename... Ts>
bool Registry::destroyComponent(Entity entity)
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	if constexpr (sizeof...(Ts) > 0)
	{
		if (m_entityFlags.find(entity.id) != m_entityFlags.end())
		{
			auto const signs = signatures<Ts...>();
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

template <typename T1, typename... Ts>
typename Registry::View<T1 const, Ts const...> Registry::view(Flags mask, Flags pattern) const
{
	return view<Registry const, T1 const, Ts const...>(this, mask, pattern);
}

template <typename T1, typename... Ts>
typename Registry::View<T1, Ts...> Registry::view(Flags mask, Flags pattern)
{
	return view<Registry, T1, Ts...>(this, mask, pattern);
}

template <typename T>
Registry::Signature Registry::signature_Impl()
{
	auto const& t = typeid(T);
	auto const index = std::type_index(t);
	auto search = s_signs.find(index);
	if (search == s_signs.end())
	{
		auto result = s_signs.emplace(index, (Signature)t.hash_code());
		ASSERT(result.second, "Insertion failure");
		search = result.first;
	}
	return search->second;
}

template <typename T, typename... Args>
Registry::Component* Registry::addComponent_Impl(Entity entity, Args... args)
{
	return addComponent_Impl(signature<T>(), std::make_unique<Model<T>>(std::forward<Args>(args)...), entity);
}

template <typename T1, typename T2, typename... Ts>
void Registry::addComponent_Impl(Entity entity)
{
	addComponent_Impl<T1>(entity);
	addComponent_Impl<T2, Ts...>(entity);
}

template <typename T>
T* Registry::component_Impl(std::unordered_map<Signature, std::unique_ptr<Component>>& compMap)
{
	if (auto search = compMap.find(signature<T>()); search != compMap.end())
	{
		return &static_cast<Model<T>*>(search->second.get())->t;
	}
	return nullptr;
}

template <typename T>
T const* Registry::component_Impl(std::unordered_map<Signature, std::unique_ptr<Component>> const& compMap)
{
	if (auto search = compMap.find(signature<T>()); search != compMap.end())
	{
		return &static_cast<Model<T>*>(search->second.get())->t;
	}
	return nullptr;
}

template <typename Th, typename T>
T* Registry::component_Impl(Th* pThis, Entity::ID id)
{
	if (auto search = pThis->m_db.find(id); search != pThis->m_db.end())
	{
		return component_Impl<T>(search->second);
	}
	return nullptr;
}

template <typename Th, typename T1, typename... Ts>
typename Registry::View<T1, Ts...> Registry::view(Th* pThis, Flags mask, Flags pattern)
{
	View<T1, Ts...> ret;
	static auto const signs = signatures<T1, Ts...>();
	std::scoped_lock<std::mutex> lock(pThis->m_mutex);
	for (auto const [id, flags] : pThis->m_entityFlags)
	{
		if ((flags & mask) == (pattern & mask))
		{
			if (auto search = pThis->m_db.find(id); search != pThis->m_db.end())
			{
				auto& compMap = search->second;
				auto checkSigns = [&compMap](auto sign) -> bool { return compMap.find(sign) != compMap.end(); };
				if (std::all_of(signs.begin(), signs.end(), checkSigns))
				{
					ret[Entity{id}] = (std::make_tuple(component_Impl<T1>(compMap), (component_Impl<Ts>(compMap))...));
				}
			}
		}
	}
	return ret;
}
} // namespace le
