#pragma once
#include <unordered_map>
#include <core/ensure.hpp>
#include <core/not_null.hpp>
#include <core/std_types.hpp>

namespace le {
///
/// \brief Type-safe mapping for (pointers to) objects used as services
///
class Services final {
  public:
	///
	/// \brief Track passed service objects
	///
	template <typename... Ts>
	static void track(not_null<Ts*>... ts) {
		(s_ts.emplace(typeID<Ts>(), ts.get()), ...);
	}
	///
	/// \brief Untrack service objects
	///
	template <typename... Ts>
	static void untrack() noexcept {
		(s_ts.erase(typeID<Ts>()), ...);
	}
	///
	/// \brief Clear tracked service objects
	///
	static void clear() noexcept { s_ts.clear(); }
	///
	/// \brief Check if a service object corresponding to T exists
	///
	template <typename T>
	static bool exists() noexcept {
		return s_ts.contains(typeID<T>());
	}
	///
	/// \brief Locate an existing service object
	///
	template <typename T>
	static T* locate() noexcept {
		if (auto it = s_ts.find(typeID<T>()); it != s_ts.end()) { return reinterpret_cast<T*>(it->second); }
		ENSURE(false, "Service not found");
		return nullptr;
	}

  private:
	template <typename T>
	static u64 typeID() noexcept {
		static u64 ret = ++s_typeID;
		return ret;
	}

	inline static std::unordered_map<u64, void*> s_ts;
	inline static u64 s_typeID = 0;
};
} // namespace le
