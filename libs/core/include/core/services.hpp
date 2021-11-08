#pragma once
#include <core/not_null.hpp>
#include <core/std_types.hpp>
#include <core/utils/error.hpp>
#include <core/utils/expect.hpp>
#include <cstdint>
#include <typeindex>
#include <unordered_map>

namespace le {
///
/// \brief Type-safe mapping for (pointers to) objects used as services
/// note: calling track() before main() is UB
///
class Services final {
	using id_t = std::type_index;

  public:
	///
	/// \brief Track passed service objects
	///
	template <typename... Ts>
	static void track(Ts*... ts) {
		((s_ts[typeID<Ts>()] = ts), ...);
	}
	///
	/// \brief Untrack service objects
	///
	template <typename... Ts>
	static void untrack() noexcept {
		((s_ts.erase(typeID<Ts>())), ...);
	}
	template <typename... Ts>
	static void untrack(Ts*...) {
		untrack<Ts...>();
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
		return std::type_index(typeid(T));
	}

	inline static std::unordered_map<id_t, void*> s_ts;
};
} // namespace le
