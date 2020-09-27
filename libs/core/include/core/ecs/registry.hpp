#pragma once
#include <memory>
#include <optional>
#include <typeindex>
#include <typeinfo>
#include <core/counter.hpp>
#include <core/flags.hpp>
#include <core/log.hpp>
#include <core/traits.hpp>
#include <core/utils.hpp>
#include <core/ecs/storage.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le::ecs
{
class Registry final
{
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
	/// \brief Entity metadata
	///
	struct Info final
	{
		std::string name;
		Flags flags;
	};

public:
	///
	/// \brief Name for logging etc
	///
	std::string m_name;
	///
	/// \brief Adjusts `io::Level` for logging on database changes (unset to disable)
	///
	std::optional<io::Level> m_logLevel = io::Level::eInfo;

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
	/// \brief Make new Entity with `T...` attached
	///
	template <typename... T>
	Spawned_t<T...> spawn(std::string name);
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
	/// \brief Obtain whether Entity exists in database
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
	template <typename... T, typename = require<!size_eq_v<1, T...>>>
	Components<T*...> attach(Entity entity);
	///
	/// \brief Remove `T0, Tn...` if attached to Entity
	///
	template <typename T0, typename... Tn>
	void detach(Entity entity);

	///
	/// \brief Obtain pointer to `T` if attached to Entity
	///
	template <typename T>
	T const* find(Entity entity) const;
	///
	/// \brief Obtain pointer to `T` if attached to Entity
	///
	template <typename T>
	T* find(Entity entity);
	///
	/// \brief Obtain pointers to `T...` if attached to Entity
	///
	template <typename... T, typename = require<!size_eq_v<1, T...>>>
	Components<T const*...> find(Entity entity) const;
	///
	/// \brief Obtain pointers to `T...` if attached to Entity
	///
	template <typename... T, typename = require<!size_eq_v<1, T...>>>
	Components<T*...> find(Entity entity);

	///
	/// \brief Obtain View of `T`
	///
	template <typename T>
	View_t<T const> view(Flags mask = Flag::eDisabled, Flags pattern = {}) const;
	///
	/// \brief Obtain View of `T`
	///
	template <typename T>
	View_t<T> view(Flags mask = Flag::eDisabled, Flags pattern = {});
	///
	/// \brief Obtain View of `T...`
	///
	template <typename... T, typename = require<!size_eq_v<1, T...>>>
	View_t<T const...> view(Flags mask = Flag::eDisabled, Flags pattern = {}) const;
	///
	/// \brief Obtain View of `T...`
	///
	template <typename... T, typename = require<!size_eq_v<1, T...>>>
	View_t<T...> view(Flags mask = Flag::eDisabled, Flags pattern = {});

	///
	/// \brief Destroy everything
	///
	void clear();
	///
	/// \brief Obtain Entity count
	///
	std::size_t size() const;

private:
	template <typename T>
	static Sign sign_Impl();

	template <typename T>
	static std::string_view name_Impl();

	bool exists_Impl(Sign sign, Entity entity) const;

	template <typename T>
	internal::Storage<T>& get_Impl();

	template <typename T>
	internal::Storage<std::decay_t<T>>* cast_Impl() const;

	template <typename Th, std::size_t N>
	static internal::Concept* minStore_Impl(Th pThis, std::array<Sign, N> const& signList);

	Entity spawn_Impl(std::string name);

	template <typename T, typename... Args>
	T& attach_Impl(Entity entity, std::string_view name, Args&&... args);

	template <typename T>
	bool detach_Impl(Entity entity, std::string_view name);

	template <typename T0, typename... Tn, typename = require<!size_lt_v<1, Tn...>>>
	void detach_Impl(Entity entity, std::string_view name);

	template <typename T>
	bool exists_Impl(Entity entity) const;

	template <typename... T, typename Th>
	static View_t<T...> view_Impl(Th pThis, Flags mask, Flags pattern);

private:
	using CMap = std::unordered_map<Sign, std::unique_ptr<internal::Concept>>;

private:
	static std::string const s_tEName;
	inline static std::unordered_map<std::type_index, Sign> s_signs;
	inline static std::unordered_map<Sign, std::string> s_names;
	inline static kt::lockable<std::mutex> s_mutex;
	inline static TCounter<ID::type> s_nextRegID = ID::null;

protected:
	// Thread-safe member mutex
	mutable kt::lockable<std::mutex> m_mutex;

private:
	CMap m_db;
	TCounter<ID::type> m_nextID = ID::null;
	ID m_regID = ID::null;
};

template <typename... T>
constexpr Spawned<T...>::operator Entity() const noexcept
{
	return entity;
}

template <typename T>
Sign Registry::sign()
{
	auto lock = s_mutex.lock();
	return sign_Impl<T>();
}

template <typename... T>
std::array<Sign, sizeof...(T)> Registry::signs()
{
	auto lock = s_mutex.lock();
	return {sign_Impl<T>()...};
}

template <typename T, typename... Args>
Spawned_t<T> Registry::spawn(std::string name, Args&&... args)
{
	auto lock = m_mutex.lock();
	auto entity = spawn_Impl(name);
	auto& comp = attach_Impl<T>(entity, name, std::forward<Args>(args)...);
	return {entity, comp};
}

template <typename... T>
Spawned_t<T...> Registry::spawn(std::string name)
{
	auto lock = m_mutex.lock();
	auto entity = spawn_Impl(name);
	if constexpr (sizeof...(T) > 0)
	{
		auto comps = Components<T&...>(attach_Impl<T>(entity, name)...);
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
	if (auto pInfo = get_Impl<Info>().find(entity))
	{
		return &attach_Impl<T>(entity, pInfo->name, std::forward<Args>(args)...);
	}
	return nullptr;
}

template <typename... T, typename>
Components<T*...> Registry::attach(Entity entity)
{
	static_assert(!size_eq_v<0, T...>, "Must pass at least one T");
	static_assert((std::is_default_constructible_v<T> && ...), "Cannot default construct T...");
	auto lock = m_mutex.lock();
	if (auto pInfo = get_Impl<Info>().find(entity))
	{
		return Components<T*...>(&attach_Impl<T>(entity, pInfo->name)...);
	}
	return {};
}

template <typename T0, typename... Tn>
void Registry::detach(Entity entity)
{
	static_assert((!std::is_same_v<Info, std::decay_t<T0>>), "Cannot destroy Info!");
	auto lock = m_mutex.lock();
	if (auto pInfo = get_Impl<Info>().find(entity))
	{
		detach_Impl<T0, Tn...>(entity, pInfo->name);
	}
}

template <typename T>
T const* Registry::find(Entity entity) const
{
	auto lock = m_mutex.lock();
	if (auto pT = cast_Impl<T>())
	{
		return pT->find(entity);
	}
	return nullptr;
}

template <typename T>
T* Registry::find(Entity entity)
{
	auto lock = m_mutex.lock();
	return get_Impl<T>().find(entity);
}

template <typename... T, typename>
Components<T const*...> Registry::find(Entity entity) const
{
	static_assert(!size_eq_v<0, T...>, "Must pass at least one T!");
	auto lock = m_mutex.lock();
	return Components<T const*...>((cast_Impl<T>() ? cast_Impl<T>()->find(entity) : nullptr)...);
}

template <typename... T, typename>
Components<T*...> Registry::find(Entity entity)
{
	static_assert(!size_eq_v<0, T...>, "Must pass at least one T!");
	auto lock = m_mutex.lock();
	return Components<T*...>(get_Impl<T>().find(entity)...);
}

template <typename T>
View_t<T const> Registry::view(Flags mask, Flags pattern) const
{
	auto lock = m_mutex.lock();
	return view_Impl<T const>(this, mask, pattern);
}

template <typename T>
View_t<T> Registry::view(Flags mask, Flags pattern)
{
	auto lock = m_mutex.lock();
	return view_Impl<T>(this, mask, pattern);
}

template <typename... T, typename>
View_t<T const...> Registry::view(Flags mask, Flags pattern) const
{
	static_assert(!size_eq_v<0, T...>, "Must pass at least one T!");
	auto lock = m_mutex.lock();
	return view_Impl<T const...>(this, mask, pattern);
}

template <typename... T, typename>
View_t<T...> Registry::view(Flags mask, Flags pattern)
{
	static_assert(!size_eq_v<0, T...>, "Must pass at least one T!");
	auto lock = m_mutex.lock();
	return view_Impl<T...>(this, mask, pattern);
}

template <typename T>
Sign Registry::sign_Impl()
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
internal::Storage<T>& Registry::get_Impl()
{
	static auto const s = sign<T>();
	auto& uT = m_db[s];
	if (!uT)
	{
		uT = std::make_unique<internal::Storage<T>>();
		uT->sign = s;
	}
	return static_cast<internal::Storage<T>&>(*uT);
}

template <typename T>
internal::Storage<std::decay_t<T>>* Registry::cast_Impl() const
{
	static auto const s = sign<std::decay_t<T>>();
	if (auto search = m_db.find(s); search != m_db.end())
	{
		return static_cast<internal::Storage<std::decay_t<T>>*>(search->second.get());
	}
	return nullptr;
}

template <typename T>
std::string_view Registry::name_Impl()
{
	static_assert(!std::is_base_of_v<internal::Concept, T>, "Invalid type!");
	auto lock = s_mutex.lock();
	auto const s = sign_Impl<T>();
	auto search = s_names.find(s);
	if (search == s_names.end())
	{
		auto result = s_names.emplace(s, utils::tName<T>());
		ASSERT(result.second, "Insertion failure");
		search = result.first;
	}
	return search->second;
}

template <typename Th, std::size_t N>
internal::Concept* Registry::minStore_Impl(Th pThis, std::array<Sign, N> const& signList)
{
	internal::Concept* pRet = nullptr;
	for (auto& s : signList)
	{
		if (auto search = pThis->m_db.find(s); search != pThis->m_db.end())
		{
			auto& uConcept = search->second;
			if (!pRet || pRet->size() > uConcept->size())
			{
				pRet = uConcept.get();
			}
		}
	}
	return pRet;
}

template <typename T, typename... Args>
T& Registry::attach_Impl(Entity entity, std::string_view name, Args&&... args)
{
	auto& storage = get_Impl<T>();
	ASSERT(!storage.find(entity), "Duplicate component!");
	if (m_logLevel && !name.empty())
	{
		LOG(*m_logLevel, "[{}] [{}] attached to [{}:{}] [{}]", m_name, name_Impl<T>(), s_tEName, entity.id, name);
	}
	return storage.attach(entity, std::forward<Args>(args)...);
}

template <typename T>
bool Registry::detach_Impl(Entity entity, std::string_view name)
{
	auto& storage = get_Impl<T>();
	if (storage.exists(entity))
	{
		if (m_logLevel && !name.empty())
		{
			LOG(*m_logLevel, "[{}] [{}] detached from [{}:{}] [{}] and destroyed", m_name, name_Impl<T>(), s_tEName, entity.id, name);
		}
		return storage.detach(entity);
	}
	return false;
}

template <typename T0, typename... Tn, typename>
void Registry::detach_Impl(Entity entity, std::string_view name)
{
	detach_Impl<T0>(entity, name);
	if constexpr (sizeof...(Tn) > 0)
	{
		detach_Impl<Tn...>(entity, name);
	}
}

template <typename T>
bool Registry::exists_Impl(Entity entity) const
{
	if (auto pT = cast_Impl<T>())
	{
		return pT->find(entity) != nullptr;
	}
	return false;
}

template <typename... T, typename Th>
View_t<T...> Registry::view_Impl(Th pThis, Flags mask, Flags pattern)
{
	View_t<T...> ret;
	if constexpr (sizeof...(T) == 1)
	{
		static auto const s = sign<T...>();
		if (auto search = pThis->m_db.find(s); search != pThis->m_db.end())
		{
			auto& storage = static_cast<internal::Storage<std::decay_t<T>...>&>(*search->second);
			for (auto& [e, t] : storage.map)
			{
				auto pStorage = pThis->template cast_Impl<Info>();
				ASSERT(pStorage, "Invariant violated!");
				auto pInfo = pStorage->find(e);
				ASSERT(pInfo, "Invariant violated");
				auto const flags = pInfo->flags;
				if ((flags & mask) == (pattern & mask))
				{
					ret.push_back({e, Components<T&...>(t)});
				}
			}
		}
	}
	else
	{
		static auto const s = signs<T...>();
		auto pMin = minStore_Impl(pThis, s);
		if (pMin)
		{
			for (auto& e : pMin->entities())
			{
				auto pStorage = pThis->template cast_Impl<Info>();
				ASSERT(pStorage, "Invariant violated!");
				auto pInfo = pStorage->find(e);
				ASSERT(pInfo, "Invariant violated");
				auto const flags = pInfo->flags;
				auto check = [e, flags, mask, pThis](Sign s) -> bool { return pThis->exists_Impl(s, e); };
				if ((flags & mask) == (pattern & mask) && std::all_of(s.begin(), s.end(), check))
				{
					ret.push_back({e, Components<T&...>(*pThis->template cast_Impl<T>()->find(e)...)});
				}
			}
		}
	}
	return ret;
}
} // namespace le::ecs
