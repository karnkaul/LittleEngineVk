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
private:
	template <typename... T>
	constexpr static bool isGreater(std::size_t const min)
	{
		return sizeof...(T) > min;
	}

public:
	///
	/// \brief Enum describing when destroyed entities are removed from the db
	///
	enum class Mode : s8
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
	using Sign = std::size_t;
	///
	/// \brief Return type of `view<T...>()`
	///
	///
	/// \brief List of `T...` components
	///
	template <typename... T>
	using Components = std::tuple<T...>;
	///
	/// \brief Return type of `spawn<T...>()`
	///
	template <typename... T>
	struct Spawned
	{
		Entity entity;
		Components<T&...> components;
	};
	template <typename... T>
	using View = std::vector<std::pair<Entity, Components<T&...>>>;

	///
	/// \brief Entity metadata
	///
	struct Info final
	{
		std::string name;
		Flags flags;
	};

public:
	Registry(Mode destroyMode = Mode::eDeferred);
	virtual ~Registry();

public:
	///
	/// \brief Obtain signature of `T`
	///
	template <typename T>
	static Sign sign();

	///
	/// \brief Obtain signatures of `T...`
	///
	template <typename... T>
	static std::array<Sign, sizeof...(T)> signs();

public:
	///
	/// \brief Make new Entity with `Ts...` attached
	///
	template <typename... Ts>
	Spawned<Ts...> spawn(std::string name);
	///
	/// \brief Make new Entity with `T(Args&&...)` attached
	///
	template <typename T, typename... Args>
	Spawned<T> spawn(std::string name, Args&&... args);
	///
	/// \brief Destroy Entity
	///
	bool destroy(Entity const& entity);
	///
	/// \brief Destroy Entity and set to default handle
	///
	bool destroy(Entity& out_entity);

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
	/// \brief Add component `T` to Entity
	///
	template <typename T, typename... Args>
	T* attach(Entity entity, Args&&... args);
	///
	/// \brief Add components `T...` to Entity
	///
	template <typename... T, typename = std::enable_if_t<isGreater<T...>(1)>>
	Components<T*...> attach(Entity entity);
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
	struct Concept
	{
		Sign sign = 0;
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
	///
	/// \brief Name for logging etc
	///
	std::string m_name;
	///
	/// \brief Adjusts `io::Level` on database changes (unset to disable)
	///
	std::optional<io::Level> m_logLevel = io::Level::eInfo;

private:
	inline static std::unordered_map<std::type_index, Sign> s_signs;
	inline static std::unordered_map<Sign, std::string> s_names;
	inline static kt::lockable<std::mutex> s_mutex;
	inline static ECSID s_nextRegID;

protected:
	using CompMap = std::unordered_map<Sign, std::unique_ptr<Concept>>;
	// Database of id => [Sign => [component]]
	std::unordered_map<Entity, CompMap, EntityHasher> m_db;
	// Thread-safe member mutex
	mutable kt::lockable<std::mutex> m_mutex;

private:
	ECSID m_nextID = 0;
	ECSID m_regID = 0;
	Mode m_destroyMode;

private:
	// _Impl functions are not thread safe; they rely on mutexes being locked
	// const helpers help with DRY for const and non-const overloads

	template <typename T>
	static Sign signature_Impl();

	template <typename T>
	static std::string_view name_Impl();

	static std::string_view name_Impl(Sign sign);

	Entity spawn_Impl(std::string name);

	using ECMap = std::unordered_map<Entity, CompMap, EntityHasher>;
	ECMap::iterator destroy_Impl(ECMap::iterator iter, Entity entity);

	template <typename T, typename... Args>
	T& attach_Impl(Entity entity, Args&&... args);

	Concept* attach_Impl(Sign sign, std::unique_ptr<Concept>&& uT, Entity entity);

	CompMap::iterator detach_Impl(CompMap& out_map, CompMap::iterator iter, Entity entity, Sign sign);

	// const helper
	template <typename T, typename Th, typename Cm>
	static T* find_Impl(Th* pThis, Cm& out_compMap);

	// const helper
	template <typename T, typename Th>
	static T* find_Impl(Th* pThis, Entity entity);

	// const helper
	template <typename Th, typename... T>
	static View<T...> view_Impl(Th* pThis, Flags mask, Flags pattern);
};

///
/// \brief Specialisation for zero components
///
template <>
struct Registry::Spawned<>
{
	Entity entity;

	constexpr operator Entity() const
	{
		return entity;
	}
};

template <typename T>
template <typename... Args>
Registry::Model<T>::Model(Args&&... args) : t(std::forward<Args>(args)...)
{
	static_assert(std::is_constructible_v<T, Args&&...>, "Invalid argument types!");
}

template <typename T>
Registry::Sign Registry::sign()
{
	auto lock = s_mutex.lock();
	return signature_Impl<T>();
}

template <typename... T>
std::array<Registry::Sign, sizeof...(T)> Registry::signs()
{
	auto lock = s_mutex.lock();
	return {{signature_Impl<T>()...}};
}

template <typename... Ts>
Registry::Spawned<Ts...> Registry::spawn(std::string name)
{
	auto lock = m_mutex.lock();
	auto entity = spawn_Impl(std::move(name));
	if constexpr (sizeof...(Ts) > 0)
	{
		auto comps = Components<Ts&...>(attach_Impl<Ts>(entity)...);
		return {entity, std::move(comps)};
	}
	else
	{
		return {entity};
	}
}

template <typename T, typename... Args>
Registry::Spawned<T> Registry::spawn(std::string name, Args&&... args)
{
	auto lock = m_mutex.lock();
	auto entity = spawn_Impl(std::move(name));
	auto& comp = attach_Impl<T>(entity, std::forward<Args>(args)...);
	return {entity, comp};
}

template <typename T, typename... Args>
T* Registry::attach(Entity entity, Args&&... args)
{
	static_assert((std::is_constructible_v<T, Args> && ...), "Cannot construct Comp with given Args...");
	auto lock = m_mutex.lock();
	if (m_db.find(entity) != m_db.end())
	{
		return &attach_Impl<T>(entity, std::forward<Args>(args)...);
	}
	return nullptr;
}

template <typename... T, typename>
Registry::Components<T*...> Registry::attach(Entity entity)
{
	auto lock = m_mutex.lock();
	if (m_db.find(entity) != m_db.end())
	{
		return Components<T*...>(&attach_Impl<T>(entity)...);
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
			for (auto sign : signs<T...>())
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
	return find_Impl<T const>(this, entity);
}

template <typename T>
T* Registry::find(Entity entity)
{
	auto lock = m_mutex.lock();
	return find_Impl<T>(this, entity);
}

template <typename... T>
typename Registry::View<T const...> Registry::view(Flags mask, Flags pattern) const
{
	return view_Impl<Registry const, T const...>(this, mask, pattern);
}

template <typename... T>
typename Registry::View<T...> Registry::view(Flags mask, Flags pattern)
{
	return view_Impl<Registry, T...>(this, mask, pattern);
}

template <typename T>
Registry::Sign Registry::signature_Impl()
{
	auto const& t = typeid(std::decay_t<T>);
	auto const index = std::type_index(t);
	auto search = s_signs.find(index);
	if (search == s_signs.end())
	{
		auto result = s_signs.emplace(index, (Sign)t.hash_code());
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
T& Registry::attach_Impl(Entity entity, Args&&... args)
{
	name_Impl<T>();
	return static_cast<Model<T>&>(*attach_Impl(sign<T>(), std::make_unique<Model<T>>(std::forward<Args>(args)...), entity)).t;
}

template <typename T, typename Th, typename Cm>
T* Registry::find_Impl(Th*, Cm& out_compMap)
{
	if (auto search = out_compMap.find(sign<T>()); search != out_compMap.end())
	{
		return &static_cast<Model<T>&>(*search->second).t;
	}
	return nullptr;
}

template <typename T, typename Th>
T* Registry::find_Impl(Th* pThis, Entity entity)
{
	if (auto search = pThis->m_db.find(entity); search != pThis->m_db.end())
	{
		return find_Impl<T>(pThis, search->second);
	}
	return nullptr;
}

template <typename Th, typename... T>
typename Registry::View<T...> Registry::view_Impl(Th* pThis, Flags mask, Flags pattern)
{
	static_assert(sizeof...(T) > 0, "Must view at least one T");
	View<T...> ret;
	static auto const s = signs<T...>();
	auto lock = pThis->m_mutex.lock();
	for (auto& [entity, compMap] : pThis->m_db)
	{
		auto pInfo = find_Impl<Info const>(pThis, compMap);
		ASSERT(pInfo, "Invariant violated");
		if ((pInfo->flags & mask) == (pattern & mask))
		{
			auto& c = compMap;
			if (std::all_of(s.begin(), s.end(), [&c](auto sign) -> bool { return c.find(sign) != c.end(); }))
			{
				ret.emplace_back(entity, Components<T&...>(*find_Impl<T>(pThis, c)...));
			}
		}
	}
	return ret;
}
} // namespace le
