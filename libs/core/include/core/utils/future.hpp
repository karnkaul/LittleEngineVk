#pragma once
#include <future>
#include <core/std_types.hpp>

namespace le {
enum class FutureState : s8 { eInvalid, eDeferred, eReady, eTimeout };
namespace utils {
///
/// \brief std::future wrapper
///
template <typename T>
struct Future {
	mutable std::future<T> future;

	FutureState state() const;
	bool busy() const;
	bool ready(bool bAllowInvalid) const;
	void wait() const;
};

template <typename T>
FutureState futureState(std::future<T> const& future) noexcept;

template <typename T>
bool ready(std::future<T> const& future) noexcept;
} // namespace utils

// impl

namespace utils {
template <typename T>
FutureState Future<T>::state() const {
	return utils::futureState(future);
}

template <typename T>
bool Future<T>::busy() const {
	return state() == FutureState::eDeferred;
}

template <typename T>
bool Future<T>::ready(bool bAllowInvalid) const {
	if (future.valid()) {
		return state() == FutureState::eReady;
	} else {
		return bAllowInvalid;
	}
}

template <typename T>
void Future<T>::wait() const {
	if (future.valid()) { future.get(); }
}
} // namespace utils

template <typename T>
FutureState utils::futureState(std::future<T> const& future) noexcept {
	if (future.valid()) {
		using namespace std::chrono_literals;
		auto const status = future.wait_for(0ms);
		switch (status) {
		default:
		case std::future_status::deferred: return FutureState::eDeferred;
		case std::future_status::ready: return FutureState::eReady;
		case std::future_status::timeout: return FutureState::eTimeout;
		}
	}
	return FutureState::eInvalid;
}

template <typename T>
bool utils::ready(std::future<T> const& future) noexcept {
	using namespace std::chrono_literals;
	return future.valid() && future.wait_for(0ms) == std::future_status::ready;
}
} // namespace le
