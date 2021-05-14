#pragma once
#include <kt/async_queue/lockable.hpp>

namespace le::utils {
///
/// \brief Wrapper for kt::lockable
///
template <bool UseMutex, typename M = std::mutex>
struct Lockable final {
	using type = M;
	static constexpr bool hasMutex = UseMutex;

	mutable kt::lockable_t<M> mutex;

	template <template <typename...> typename L = std::scoped_lock, typename... Args>
	decltype(mutex.template lock<L, Args...>()) lock() const {
		return mutex.template lock<L, Args...>();
	}
};

///
/// \brief Specialisation for Dummy lock
///
template <typename M>
struct Lockable<false, M> {
	using type = void;
	static constexpr bool hasMutex = false;

	struct Dummy final {
		///
		/// \brief Custom destructor to suppress unused variable warnings
		///
		~Dummy() {}
	};

	template <template <typename...> typename = std::scoped_lock, typename...>
	Dummy lock() const {
		return {};
	}
};

} // namespace le::utils
