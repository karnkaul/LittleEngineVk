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
#include <core/counter.hpp>
#include <core/flags.hpp>
#include <core/log_config.hpp>
#include <core/span.hpp>
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
	static constexpr bool isGreater(std::size_t const min);

public:
	///
	/// \brief Desired flags can be combined with a mask as per-Entity filters for `view()`
	///
	enum class Flag : s8
	{
		eDisabled,
		eDebug,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;
	///
	/// \brief Hash code of component type
	///
	using Sign = std::size_t;
	///
	/// \brief List of `T...` components
	///
	template <typename... T>
	using Components = std::tuple<T...>;
	///
	/// \brief Return type wrapper for `spawn<T...>()`
	///
	template <typename... T>
	struct Spawned
	{
		using type = Spawned<T...>;

		Entity entity;
		Components<T&...> components;

		constexpr operator Entity() const noexcept;
	};
	template <typename... T>
	using Spawned_t = typename Spawned<T...>::type;
	///
	/// \brief Return type of `view<T...>()`
	///
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
	Registry();
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
	/// \brief Make new Entity with `T(Args&&...)` attached
	///
	template <typename T, typename... Args>
	Spawned_t<T> spawn(std::string name, Args&&... args);
	///
	/// \brief Make new Entity with `Ts...` attached
	///
	template <typename... Ts>
	Spawned_t<Ts...> spawn(std::string name);
	///
	/// \brief Destroy Entity
	///
	bool destroy(Entity entity);
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
	/// \brief Add `T` to Entity
	///
	template <typename T, typename... Args>
	T* attach(Entity entity, Args&&... args);
	///
	/// \brief Add `T...` to Entity
	///
	template <typename... T, typename = std::enable_if_t<isGreater<T...>(1)>>
	Components<T*...> attach(Entity entity);
	///
	/// \brief Remove `T...` if attached to Entity
	///
	template <typename... T>
	void detach(Entity entity);

	///
	/// \brief Obtain pointer to T if attached to Entity
	///
	template <typename T>
	T const* find(Entity entity) const;
	///
	/// \brief Obtain pointer to T if attached to Entity
	///
	template <typename T>
	T* find(Entity entity);

	///
	/// \brief Obtain View of `T...`
	///
	template <typename... T>
	View<T const...> view(Flags mask = Flag::eDisabled, Flags pattern = {}) const;
	///
	/// \brief Obtain View of `T...`
	///
	template <typename... T>
	View<T...> view(Flags mask = Flag::eDisabled, Flags pattern = {});

	///
	/// \brief Destroy everything
	///
	void clear();
	///
	/// \brief Obtain Entity count
	///
	std::size_t size() const;

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

private:
	using EMap = std::unordered_map<Entity, std::unique_ptr<Concept>, EntityHasher>;
	using CMap = std::unordered_map<Sign, EMap>;

private:
	inline static std::unordered_map<std::type_index, Sign> s_signs;
	inline static std::unordered_map<Sign, std::string> s_names;
	inline static kt::lockable<std::mutex> s_mutex;
	inline static TCounter<ECSID::type> s_nextRegID = ECSID::null;

protected:
	// Thread-safe member mutex
	mutable kt::lockable<std::mutex> m_mutex;

private:
	CMap m_db;
	TCounter<ECSID::type> m_nextID = ECSID::null;
	ECSID m_regID = ECSID::null;

private:
	template <typename T>
	static Sign sign_Impl();

	template <typename T>
	static std::string_view name_Impl();

	static std::string_view name_Impl(Sign sign);

	template <typename R, typename Th>
	static R* minMap_Impl(Th pThis, Span<Sign> signList);

	Entity spawn_Impl(std::string name);

	template <typename T, typename... Args>
	T& attach_Impl(Entity entity, EMap& out_emap, Args&&... args);

	Concept* attach_Impl(Sign sign, EMap& out_emap, std::unique_ptr<Concept>&& uT, Entity entity);

	EMap::iterator detach_Impl(EMap& out_map, EMap::iterator iter, Sign sign, std::string_view name);

	bool exists_Impl(Sign s, Entity entity) const;

	// const helper
	template <typename T, typename Th, typename Em>
	static T* find_Impl(Th* pThis, Em& out_emap, Entity entity);

	// const helper
	template <typename T, typename Th>
	static T* find_Impl(Th* pThis, Entity entity);

	// const helper
	template <typename Em, typename Th, typename... T>
	static View<T...> view_Impl(Th* pThis, Flags mask, Flags pattern);
};

template <typename... T>
constexpr bool Registry::isGreater(std::size_t const min)
{
	return sizeof...(T) > min;
}

template <typename... T>
constexpr Registry::Spawned<T...>::operator Entity() const noexcept
{
	return entity;
}

///
/// \brief Specialisation for zero components
///
template <>
struct Registry::Spawned<>
{
	using type = Entity;
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
	return sign_Impl<T>();
}

template <typename... T>
std::array<Registry::Sign, sizeof...(T)> Registry::signs()
{
	auto lock = s_mutex.lock();
	return {sign_Impl<T>()...};
}

template <typename T, typename... Args>
typename Registry::Spawned_t<T> Registry::spawn(std::string name, Args&&... args)
{
	auto lock = m_mutex.lock();
	auto entity = spawn_Impl(std::move(name));
	auto& comp = attach_Impl<T>(entity, m_db[sign_Impl<T>()], std::forward<Args>(args)...);
	return {entity, comp};
}

template <typename... Ts>
typename Registry::Spawned_t<Ts...> Registry::spawn(std::string name)
{
	auto lock = m_mutex.lock();
	auto entity = spawn_Impl(std::move(name));
	if constexpr (sizeof...(Ts) > 0)
	{
		auto comps = Components<Ts&...>(attach_Impl<Ts>(entity, m_db[sign_Impl<Ts>()])...);
		return {entity, std::move(comps)};
	}
	else
	{
		return entity;
	}
}

template <typename T, typename... Args>
T* Registry::attach(Entity entity, Args&&... args)
{
	static_assert(std::is_constructible_v<T, Args...>, "Cannot construct T with given Args...");
	auto lock = m_mutex.lock();
	if (find_Impl<Info>(this, m_db[sign_Impl<Info>()], entity))
	{
		auto& emap = m_db[sign_Impl<T>()];
		return &attach_Impl<T>(entity, emap, std::forward<Args>(args)...);
	}
	return nullptr;
}

template <typename... T, typename>
typename Registry::Components<T*...> Registry::attach(Entity entity)
{
	static_assert(sizeof...(T) > 0, "Must pass at least one T");
	static_assert((std::is_default_constructible_v<T> && ...), "Cannot default construct T...");
	auto lock = m_mutex.lock();
	if (find_Impl<Info>(this, m_db[sign_Impl<Info>()], entity))
	{
		return Components<T*...>(&attach_Impl<T>(entity, m_db[sign_Impl<T>()])...);
	}
	return {};
}

template <typename... T>
void Registry::detach(Entity entity)
{
	static_assert((!std::is_same_v<Info, std::decay_t<T>> && ...), "Cannot destroy Info!");
	auto lock = m_mutex.lock();
	auto pInfo = find_Impl<Info const>(this, m_db[sign_Impl<Info>()], entity);
	for (auto sign : signs<T...>())
	{
		if (auto cSearch = m_db.find(sign); cSearch != m_db.end())
		{
			auto& emap = cSearch->second;
			if (auto eSearch = emap.find(entity); eSearch != emap.end())
			{
				ASSERT(pInfo, "Invariant violated!");
				detach_Impl(emap, eSearch, sign, pInfo->name);
			}
		}
	}
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
	auto lock = m_mutex.lock();
	return Registry::view_Impl<EMap const, Registry const, T const...>(this, mask, pattern);
}

template <typename... T>
typename Registry::View<T...> Registry::view(Flags mask, Flags pattern)
{
	auto lock = m_mutex.lock();
	return Registry::view_Impl<EMap, Registry, T...>(this, mask, pattern);
}

template <typename T>
Registry::Sign Registry::sign_Impl()
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
	auto const sign = sign_Impl<T>();
	auto search = s_names.find(sign);
	if (search == s_names.end())
	{
		auto result = s_names.emplace(sign, utils::tName<T>());
		ASSERT(result.second, "Insertion failure");
		search = result.first;
	}
	return search->second;
}

template <typename R, typename Th>
R* Registry::minMap_Impl(Th pThis, Span<Sign> signList)
{
	R* pRet = nullptr;
	for (auto& s : signList)
	{
		if (auto search = pThis->m_db.find(s); search != pThis->m_db.end())
		{
			auto& emap = search->second;
			if (!pRet || pRet->size() > emap.size())
			{
				pRet = &emap;
			}
		}
	}
	return pRet;
}

template <typename T, typename... Args>
T& Registry::attach_Impl(Entity entity, EMap& out_emap, Args&&... args)
{
	name_Impl<T>();
	auto pConcept = attach_Impl(sign_Impl<T>(), out_emap, std::make_unique<Model<T>>(std::forward<Args>(args)...), entity);
	return static_cast<Model<T>&>(*pConcept).t;
}

template <typename T, typename Th, typename Em>
T* Registry::find_Impl(Th*, Em& out_emap, Entity entity)
{
	auto search = out_emap.find(entity);
	if (search != out_emap.end())
	{
		return std::addressof(static_cast<Model<T>&>(*search->second).t);
	}
	return nullptr;
}

template <typename T, typename Th>
T* Registry::find_Impl(Th* pThis, Entity entity)
{
	if (auto search = pThis->m_db.find(sign_Impl<T>()); search != pThis->m_db.end())
	{
		return find_Impl<T>(pThis, search->second, entity);
	}
	return nullptr;
}

template <typename Em, typename Th, typename... T>
typename Registry::View<T...> Registry::view_Impl(Th* pThis, Flags mask, Flags pattern)
{
	static auto const s = signs<T...>();
	View<T...> ret;
	auto pMap = minMap_Impl<Em>(pThis, s);
	if (pMap)
	{
		for (auto& [entity, uComp] : *pMap)
		{
			auto pImpl = find_Impl<Info>(pThis, entity);
			ASSERT(pImpl, "Invariant violated");
			auto const flags = pImpl->flags;
			if ((flags & mask) == (pattern & mask))
			{
				if (std::all_of(s.begin(), s.end(), [e = entity, pThis](auto sign) -> bool { return pThis->exists_Impl(sign, e); }))
				{
					ret.emplace_back(entity, Components<T&...>(*find_Impl<T>(pThis, entity)...));
				}
			}
		}
	}
	return ret;
}
} // namespace le
