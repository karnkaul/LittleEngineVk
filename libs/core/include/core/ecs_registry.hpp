#pragma once
#include <algorithm>
#include <deque>
#include <memory>
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
#include <core/zero.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le
{
using ECSID = TZero<u64>;

///
/// \brief Entity is a glorified, type-safe combination of ID and registryID
///
struct Entity final
{
	ECSID id;
	ECSID regID;
};

bool operator==(Entity lhs, Entity rhs);
bool operator!=(Entity lhs, Entity rhs);

struct EntityHasher final
{
	std::size_t operator()(Entity const& entity) const;
};

class Registry
{
public:
	///
	/// \brief Enum describing when destroyed entities are removed from the db
	///
	enum class DestroyMode : s8
	{
		eDeferred,	// destroyEntity() sets eDestroyed
		eImmediate, // destroyEntity() erases Entity
		eCOUNT_,
	};

	///
	/// \brief Desired flags can be combined with a mask as per-Entity filters for `view()`
	///
	enum class Flag : s8
	{
		eDisabled,
		eDestroyed,
		eDebug,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	///
	/// \brief Hash code of component type
	///
	using Signature = std::size_t;

	///
	/// \brief Return type of `view()`
	///
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
	std::string m_name;

private:
	// Shared cache of computed signs
	inline static std::unordered_map<std::type_index, Signature> s_signs;
	// Thread-safe static mutex
	inline static kt::lockable<std::mutex> s_mutex;
	inline static ECSID s_nextRegID;

public:
	///
	/// \brief Adjusts `io::Level` on database changes (unset to disable)
	///
	std::optional<io::Level> m_logLevel = io::Level::eInfo;

protected:
	std::unordered_map<Entity, Flags, EntityHasher> m_entityFlags; // Every Entity has a Flags, even if not in m_db
	std::unordered_map<Entity, std::string, EntityHasher> m_entityNames;
	std::unordered_map<Signature, std::string> m_componentNames;
	// Database of id => [Sign => [component]]
	std::unordered_map<Entity, std::unordered_map<Signature, std::unique_ptr<Component>>, EntityHasher> m_db;
	// Thread-safe member mutex
	mutable kt::lockable<std::mutex> m_mutex;

private:
	ECSID m_nextID = 0;
	ECSID m_regID = 0;
	DestroyMode m_destroyMode;

public:
	Registry(DestroyMode destroyMode = DestroyMode::eDeferred);
	virtual ~Registry();

public:
	///
	/// \brief Obtain signature of T
	///
	template <typename T>
	static Signature signature();

	///
	/// \brief Obtain signatures of Ts
	///
	template <typename... Ts>
	static std::array<Signature, sizeof...(Ts)> signatures();

public:
	///
	/// \brief Make new Entity
	///
	template <typename... Ts>
	Entity spawnEntity(std::string name);

	///
	/// \brief Toggle Enabled flag
	///
	bool setEnabled(Entity entity, bool bEnabled);
	///
	/// \brief Toggle Debug flag
	///
	bool setDebug(Entity entity, bool bDebug);
	///
	/// \brief Obtain whether Enabled flag is set
	///
	bool isEnabled(Entity entity) const;
	///
	/// \brief Obtain whether Destroyed flag is not set
	///
	bool isAlive(Entity entity) const;
	///
	/// \brief Obtain whether Debug flag is set
	///
	bool isDebugSet(Entity entity) const;

	///
	/// \brief Destroy Entity
	///
	bool destroyEntity(Entity const& entity);
	///
	/// \brief Destroy Entity and set to default handle
	///
	bool destroyEntity(Entity& out_entity);

	///
	/// \brief Add Component to Entity
	///
	template <typename T, typename... Args>
	T* addComponent(Entity entity, Args&&... args);

	///
	/// \brief Add Components to Entity
	///
	template <typename T1, typename T2, typename... Ts>
	void addComponent(Entity entity);

	///
	/// \brief Obtain pointer to Component attached to Entity
	///
	template <typename T>
	T const* component(Entity entity) const;

	///
	/// \brief Obtain pointer to Component attached to Entity
	///
	template <typename T>
	T* component(Entity entity);

	///
	/// \brief Destroy Component if attached to Entity
	///
	template <typename... Ts>
	bool destroyComponent(Entity entity);

	///
	/// \brief Obtain View of T1, Ts...
	///
	template <typename T1, typename... Ts>
	View<T1 const, Ts const...> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {}) const;
	///
	/// \brief Obtain View of T1, Ts...
	///
	template <typename T1, typename... Ts>
	View<T1, Ts...> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {});

	///
	/// \brief Destroy all entities with Destroy flag set
	///
	void flush();
	///
	/// \brief Destroy everything
	///
	void clear();

	///
	/// \brief Obtain total entities spawned
	///
	std::size_t entityCount() const;
	///
	/// \brief Obtain Entity name
	///
	std::string_view entityName(Entity entity) const;

private:
	// _Impl functions are not thread safe; they rely on mutexes being locked
	// const helpers help with DRY for const and non-const overloads

	template <typename T>
	static Signature signature_Impl();

	Entity spawnEntity_Impl(std::string name);

	using EFMap = std::unordered_map<Entity, Flags, EntityHasher>;
	EFMap::iterator destroyEntity_Impl(EFMap::iterator iter, Entity entity);

	template <typename T, typename... Args>
	Component* addComponent_Impl(Entity entity, Args&&... args);

	template <typename T1, typename T2, typename... Ts>
	void addComponent_Impl(Entity entity);

	Component* addComponent_Impl(Signature sign, std::unique_ptr<Component>&& uT, Entity entity);

	template <typename T>
	static T* component_Impl(std::unordered_map<Signature, std::unique_ptr<Component>>& compMap);

	template <typename T>
	static T const* component_Impl(std::unordered_map<Signature, std::unique_ptr<Component>> const& compMap);

	void destroyComponent_Impl(Component const* pComponent, Entity entity);
	bool destroyComponent_Impl(Entity entity);

	// const helper
	template <typename Th, typename T>
	static T* component_Impl(Th* pThis, Entity entity);

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
	auto lock = s_mutex.lock();
	return signature_Impl<T>();
}

template <typename... Ts>
std::array<Registry::Signature, sizeof...(Ts)> Registry::signatures()
{
	auto lock = s_mutex.lock();
	return std::array<Signature, sizeof...(Ts)>{signature_Impl<Ts>()...};
}

template <typename... Ts>
Entity Registry::spawnEntity(std::string name)
{
	auto lock = m_mutex.lock();
	auto entity = spawnEntity_Impl(std::move(name));
	if constexpr (sizeof...(Ts) > 0)
	{
		addComponent_Impl<Ts...>(entity);
	}
	return entity;
}

template <typename T, typename... Args>
T* Registry::addComponent(Entity entity, Args&&... args)
{
	static_assert((std::is_constructible_v<T, Args> && ...), "Cannot construct Comp with given Args...");
	auto lock = m_mutex.lock();
	if (m_entityFlags.find(entity) != m_entityFlags.end())
	{
		return &static_cast<Model<T>*>(addComponent_Impl<T>(entity, std::forward<Args>(args)...))->t;
	}
	return nullptr;
}

template <typename T1, typename T2, typename... Ts>
void Registry::addComponent(Entity entity)
{
	auto lock = m_mutex.lock();
	if (m_entityFlags.find(entity) != m_entityFlags.end())
	{
		addComponent_Impl<T1, T2, Ts...>(entity);
	}
	return;
}

template <typename T>
T const* Registry::component(Entity entity) const
{
	auto lock = m_mutex.lock();
	return component_Impl<Registry const, T const>(this, entity);
}

template <typename T>
T* Registry::component(Entity entity)
{
	auto lock = m_mutex.lock();
	return component_Impl<Registry, T>(this, entity);
}

template <typename... Ts>
bool Registry::destroyComponent(Entity entity)
{
	auto lock = m_mutex.lock();
	if constexpr (sizeof...(Ts) > 0)
	{
		if (m_entityFlags.find(entity) != m_entityFlags.end())
		{
			auto const signs = signatures<Ts...>();
			for (auto sign : signs)
			{
				if (auto search = m_db[entity].find(sign); search != m_db[entity].end())
				{
					destroyComponent_Impl(search->second.get(), entity);
				}
			}
			return true;
		}
		return false;
	}
	else
	{
		return destroyComponent_Impl(entity);
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
Registry::Component* Registry::addComponent_Impl(Entity entity, Args&&... args)
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
T* Registry::component_Impl(Th* pThis, Entity entity)
{
	if (auto search = pThis->m_db.find(entity); search != pThis->m_db.end())
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
	auto lock = pThis->m_mutex.lock();
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
