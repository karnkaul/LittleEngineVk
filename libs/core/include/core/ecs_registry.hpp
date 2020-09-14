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
#include <core/utils.hpp>
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
	/// \brief List of `T...` components
	///
	template <typename... T>
	using Components = std::tuple<T...>;
	///
	/// \brief Return type of `view<T...>()`
	///
	template <typename... T>
	using View = std::unordered_map<Entity, Components<T&...>, EntityHasher>;
	///
	/// \brief Return type of `spawn<T...>()`
	///
	template <typename... T>
	using Spawned = std::pair<Entity, Components<T&...>>;

	///
	/// \brief Entity metadata
	///
	struct Info final
	{
		std::string name;
		Flags flags;
	};

private:
	struct Concept
	{
		Signature sign = 0;
		virtual ~Concept();
	};

	template <typename T>
	struct Model final : Concept
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
	inline static std::unordered_map<Signature, std::string> s_names;
	;
	// Thread-safe static mutex
	inline static kt::lockable<std::mutex> s_mutex;
	inline static ECSID s_nextRegID;

public:
	///
	/// \brief Adjusts `io::Level` on database changes (unset to disable)
	///
	std::optional<io::Level> m_logLevel = io::Level::eInfo;

protected:
	using CompMap = std::unordered_map<Signature, std::unique_ptr<Concept>>;
	// Database of id => [Sign => [component]]
	std::unordered_map<Entity, CompMap, EntityHasher> m_db;
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
	/// \brief Obtain signature of `T`
	///
	template <typename T>
	static Signature signature();

	///
	/// \brief Obtain signatures of `T...`
	///
	template <typename... T>
	static std::array<Signature, sizeof...(T)> signatures();

public:
	///
	/// \brief Make new Entity
	///
	Entity spawn(std::string name);
	///
	/// \brief Make new Entity with `T(Args&&...)` attached
	///
	template <typename T, typename... Args>
	Spawned<T> spawn(std::string name, Args&&... args);
	///
	/// \brief Make new Entity with `T0, Ts...` attached
	///
	template <typename T0, typename... Ts>
	Spawned<T0, Ts...> spawn(std::string name);

	///
	/// \brief Toggle Enabled flag
	///
	bool enable(Entity entity, bool bEnabled);
	///
	/// \brief Obtain whether Enabled flag is set
	///
	bool enabled(Entity entity) const;
	///
	/// \brief Obtain whether Destroyed flag is not set
	///
	bool exists(Entity entity) const;
	///
	/// \brief Obtain Entity name
	///
	std::string_view name(Entity entity) const;
	///
	/// \brief Obtain info for entity
	///
	Info* info(Entity entity);
	///
	/// \brief Obtain info for entity
	///
	Info const* info(Entity entity) const;

	///
	/// \brief Destroy Entity
	///
	bool destroy(Entity const& entity);
	///
	/// \brief Destroy Entity and set to default handle
	///
	bool destroy(Entity& out_entity);

	///
	/// \brief Add component `T` to Entity
	///
	template <typename T, typename... Args>
	T* attach(Entity entity, Args&&... args);
	///
	/// \brief Add components `T0, T1, Ts...` to Entity
	///
	template <typename T0, typename T1, typename... Ts>
	Components<T0*, T1*, Ts*...> attach(Entity entity);

	///
	/// \brief Remove `T...` components if attached to Entity
	///
	template <typename... T>
	bool detach(Entity entity);

	///
	/// \brief Obtain pointer to component if attached to Entity
	///
	template <typename T>
	T const* find(Entity entity) const;
	///
	/// \brief Obtain pointer to component if attached to Entity
	///
	template <typename T>
	T* find(Entity entity);

	///
	/// \brief Obtain View of `T...`
	///
	template <typename... T>
	View<T const...> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {}) const;
	///
	/// \brief Obtain View of `T...`
	///
	template <typename... T>
	View<T...> view(Flags mask = Flag::eDestroyed | Flag::eDisabled, Flags pattern = {});

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
	std::size_t size() const;

private:
	// _Impl functions are not thread safe; they rely on mutexes being locked
	// const helpers help with DRY for const and non-const overloads

	template <typename T>
	static Signature signature_Impl();

	template <typename T>
	static std::string_view name_Impl();

	static std::string_view name_Impl(Signature sign);

	Entity spawn_Impl(std::string name);

	template <typename T, typename... Args>
	T* attach_Impl(Entity entity, Args&&... args);

	Concept* attach_Impl(Signature sign, std::unique_ptr<Concept>&& uT, Entity entity);

	using ECMap = std::unordered_map<Entity, CompMap, EntityHasher>;
	ECMap::iterator destroy_Impl(ECMap::iterator iter, Entity entity);

	CompMap::iterator detach_Impl(CompMap& out_map, CompMap::iterator iter, Entity entity, Signature sign);

	template <typename T>
	static T* find_Impl(CompMap& compMap);

	template <typename T>
	static T const* find_Impl(CompMap const& compMap);

	// const helper
	template <typename Th, typename T>
	static T* find_Impl(Th* pThis, Entity entity);

	// const helper
	template <typename Th, typename... T>
	static View<T...> view(Th* pThis, Flags mask, Flags pattern);
};

template <typename T>
template <typename... Args>
Registry::Model<T>::Model(Args&&... args) : t(std::forward<Args>(args)...)
{
	static_assert(std::is_constructible_v<T, Args&&...>, "Invalid argument types!");
}

template <typename T>
Registry::Signature Registry::signature()
{
	auto lock = s_mutex.lock();
	return signature_Impl<T>();
}

template <typename... T>
std::array<Registry::Signature, sizeof...(T)> Registry::signatures()
{
	auto lock = s_mutex.lock();
	return {{signature_Impl<T>()...}};
}

template <typename T, typename... Args>
Registry::Spawned<T> Registry::spawn(std::string name, Args&&... args)
{
	auto lock = m_mutex.lock();
	auto entity = spawn_Impl(std::move(name));
	auto pComp = attach_Impl<T>(entity, std::forward<Args>(args)...);
	return {entity, *pComp};
}

template <typename T0, typename... Ts>
Registry::Spawned<T0, Ts...> Registry::spawn(std::string name)
{
	auto lock = m_mutex.lock();
	auto entity = spawn_Impl(std::move(name));
	auto pComp = attach_Impl<T0>(entity);
	auto comps = Components<T0&, Ts&...>(*pComp, attach_Impl<Ts>(entity)...);
	return std::make_pair(entity, std::move(comps));
}

template <typename T, typename... Args>
T* Registry::attach(Entity entity, Args&&... args)
{
	static_assert((std::is_constructible_v<T, Args> && ...), "Cannot construct Comp with given Args...");
	auto lock = m_mutex.lock();
	if (m_db.find(entity) != m_db.end())
	{
		return attach_Impl<T>(entity, std::forward<Args>(args)...);
	}
	return nullptr;
}

template <typename T0, typename T1, typename... Ts>
Registry::Components<T0*, T1*, Ts*...> Registry::attach(Entity entity)
{
	auto lock = m_mutex.lock();
	if (m_db.find(entity) != m_db.end())
	{
		return Components<T0*, T1*, Ts*...>(attach_Impl<T0>(entity), attach_Impl<T1>(entity), attach_Impl<Ts>(entity)...);
	}
	return {};
}

template <typename... T>
bool Registry::detach(Entity entity)
{
	if constexpr (sizeof...(T) > 0)
	{
		static_assert((... && !std::is_same_v<T, Info>), "Cannot destroy Info!");
		auto lock = m_mutex.lock();
		if (auto search = m_db.find(entity); search != m_db.end())
		{
			auto& comps = search->second;
			auto const signs = signatures<T...>();
			for (auto sign : signs)
			{
				if (auto search = comps.find(sign); search != comps.end())
				{
					detach_Impl(comps, search, entity, sign);
				}
			}
			return true;
		}
	}
	return false;
}

template <typename T>
T const* Registry::find(Entity entity) const
{
	auto lock = m_mutex.lock();
	return find_Impl<Registry const, T const>(this, entity);
}

template <typename T>
T* Registry::find(Entity entity)
{
	auto lock = m_mutex.lock();
	return find_Impl<Registry, T>(this, entity);
}

template <typename... T>
typename Registry::View<T const...> Registry::view(Flags mask, Flags pattern) const
{
	return view<Registry const, T const...>(this, mask, pattern);
}

template <typename... T>
typename Registry::View<T...> Registry::view(Flags mask, Flags pattern)
{
	return view<Registry, T...>(this, mask, pattern);
}

template <typename T>
Registry::Signature Registry::signature_Impl()
{
	auto const& t = typeid(std::decay_t<T>);
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

template <typename T>
std::string_view Registry::name_Impl()
{
	static_assert(!std::is_base_of_v<Concept, T>, "Invalid type!");
	auto const sign = signature_Impl<T>();
	auto lock = s_mutex.lock();
	auto search = s_names.find(sign);
	if (search == s_names.end())
	{
		auto result = s_names.emplace(sign, utils::tName<T>());
		ASSERT(result.second, "Insertion failure");
		search = result.first;
	}
	return search->second;
}

template <typename T, typename... Args>
T* Registry::attach_Impl(Entity entity, Args&&... args)
{
	name_Impl<T>();
	return &static_cast<Model<T>&>(*attach_Impl(signature<T>(), std::make_unique<Model<T>>(std::forward<Args>(args)...), entity)).t;
}

template <typename T>
T* Registry::find_Impl(CompMap& compMap)
{
	if (auto search = compMap.find(signature<T>()); search != compMap.end())
	{
		return &static_cast<Model<T>&>(*search->second).t;
	}
	return nullptr;
}

template <typename T>
T const* Registry::find_Impl(CompMap const& compMap)
{
	if (auto search = compMap.find(signature<T>()); search != compMap.end())
	{
		return &static_cast<Model<T>&>(*search->second).t;
	}
	return nullptr;
}

template <typename Th, typename T>
T* Registry::find_Impl(Th* pThis, Entity entity)
{
	if (auto search = pThis->m_db.find(entity); search != pThis->m_db.end())
	{
		return find_Impl<T>(search->second);
	}
	return nullptr;
}

template <typename Th, typename... T>
typename Registry::View<T...> Registry::view(Th* pThis, Flags mask, Flags pattern)
{
	static_assert(sizeof...(T) > 0, "Must view at least one T");
	View<T...> ret;
	static auto const signs = signatures<T...>();
	auto lock = pThis->m_mutex.lock();
	for (auto& [entity, compMap] : pThis->m_db)
	{
		auto pInfo = find_Impl<Info const>(compMap);
		ASSERT(pInfo, "Invariant violated");
		if ((pInfo->flags & mask) == (pattern & mask))
		{
			auto& c = compMap;
			if (std::all_of(signs.begin(), signs.end(), [&c](auto sign) -> bool { return c.find(sign) != c.end(); }))
			{
				ret.emplace(Entity{entity}, Components<T&...>(*find_Impl<T>(c)...));
			}
		}
	}
	return ret;
}
} // namespace le
