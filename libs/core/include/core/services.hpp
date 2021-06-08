#pragma once
#include <cstdint>
#include <core/ensure.hpp>
#include <core/not_null.hpp>
#include <core/std_types.hpp>

namespace le {
template <std::size_t N>
class ServiceLocator;

using Services = ServiceLocator<16>;

///
/// \brief Type-safe mapping for (pointers to) objects used as services
///
template <std::size_t N>
class ServiceLocator final {
  public:
	static constexpr std::size_t maxServices = N;

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
		((s_ts[typeID<Ts>()] = nullptr), ...);
	}
	///
	/// \brief Clear tracked service objects
	///
	static void clear() noexcept {
		for (auto& p : s_ts) { p = nullptr; }
	}
	///
	/// \brief Check if a service object corresponding to T exists
	///
	template <typename T>
	[[nodiscard]] static bool exists() noexcept {
		return s_ts[typeID<T>()] != nullptr;
	}
	///
	/// \brief Locate an existing service object
	///
	template <typename T>
	[[nodiscard]] static T* locate() noexcept {
		ENSURE(exists<T>(), "Service not found");
		return reinterpret_cast<T*>(s_ts[typeID<T>()]);
	}

  private:
	template <typename T>
	static std::size_t typeID() noexcept {
		static std::size_t ret = ++s_typeID;
		ENSURE(ret < maxServices, "Max services exceeded");
		return ret;
	}

	inline static void* s_ts[N] = {};
	inline static std::size_t s_typeID = 0;
};
} // namespace le
