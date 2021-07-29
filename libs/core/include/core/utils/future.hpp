#pragma once
#include <future>
#include <core/std_types.hpp>

namespace le {
enum class FutureState : s8 { eInvalid, eDeferred, eReady, eTimeout };
namespace utils {
template <typename T>
FutureState futureState(std::future<T> const& future) noexcept;

template <typename T>
bool ready(std::future<T> const& future) noexcept;

template <typename T>
struct FutureBase {
	mutable std::future<T> future;

	FutureBase(std::future<T> future = {}) noexcept : future(std::move(future)) {}

	FutureState state() const { return futureState(future); }
	bool busy() const { return state() == FutureState::eDeferred; }
	bool ready(bool allowInvalid) const { return (future.valid() && state() == FutureState::eReady) || allowInvalid; }
	void wait() const {
		if (this->future.valid()) { this->future.get(); }
	}
};

///
/// \brief Convenience wrapper over std::future<T>
///
template <typename T>
struct Future : FutureBase<T> {
	using FutureBase<T>::FutureBase;

	T get(T const& fallback = {}) const { return this->future.valid() ? this->future.get() : fallback; }
};
///
/// \brief Convenience wrapper over std::future<void>
///
template <>
struct Future<void> : FutureBase<void> {
	using FutureBase::FutureBase;
};
} // namespace utils

// impl

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
