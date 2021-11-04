#pragma once
#include <cstdint>
#include <typeinfo>
#include <unordered_map>
#include <core/not_null.hpp>
#include <core/std_types.hpp>
#include <core/utils/error.hpp>
#include <core/utils/expect.hpp>

namespace le {
///
/// \brief CRTP base class for RAII service types
/// note: creating multiple instances of a service type is UB
///
template <typename T>
class Service;

///
/// \brief Type-safe mapping for (pointers to) objects used as services
/// note: calling track() before main() is UB
///
class Services final {
	using id_t = std::size_t;

  public:
	///
	/// \brief Track passed service objects
	///
	template <typename... Ts>
	static void track(not_null<Ts*>... ts) {
		((s_ts[typeID<Ts>()] = ts.get()), ...);
	}
	///
	/// \brief Untrack service objects
	///
	template <typename... Ts>
	static void untrack() noexcept {
		((s_ts.erase(typeID<Ts>())), ...);
	}
	///
	/// \brief Check if a service object corresponding to T exists
	///
	template <typename T>
	[[nodiscard]] static bool exists() noexcept {
		return s_ts.contains(typeID<T>());
	}
	///
	/// \brief Obtain an existing service object
	///
	template <typename T>
	[[nodiscard]] static not_null<T*> get() noexcept(false) {
		ENSURE(exists<T>(), "Service not found");
		return find<T>();
	}
	///
	/// \brief Locate a service object if present
	///
	template <typename T>
	[[nodiscard]] static T* find() noexcept {
		return reinterpret_cast<T*>(s_ts[typeID<T>()]);
	}
	///
	/// \brief Obtain number of services being tracked
	///
	static std::size_t count() noexcept { return s_ts.size(); }
	///
	/// \brief Clear tracked service objects
	///
	static void clear() noexcept { s_ts.clear(); }

  private:
	template <typename T>
	static id_t typeID() noexcept {
		return typeid(T).hash_code();
	}

	inline static std::unordered_map<id_t, void*> s_ts;
};

// impl
template <typename T>
class Service {
	using storage_t = T*;

  public:
	using type = T;

	Service() {
		ENSURE(s_inst == nullptr, "Duplicate service instance");
		Services::track<T>(s_inst = m_inst = static_cast<storage_t>(this));
	}

	Service(Service&& rhs) noexcept : m_inst(std::exchange(rhs.m_inst, nullptr)) {}

	Service& operator=(Service&& rhs) noexcept {
		if (&rhs != this) {
			ENSURE(m_inst == nullptr, "Duplicate service instance");
			m_inst = std::exchange(rhs.m_inst, nullptr);
		}
		return *this;
	}

	~Service() noexcept {
		if (m_inst) {
			EXPECT(m_inst == s_inst);
			Services::untrack<T>();
			s_inst = {};
		}
	}

  private:
	inline static storage_t s_inst = {};
	storage_t m_inst = {};
};
} // namespace le
