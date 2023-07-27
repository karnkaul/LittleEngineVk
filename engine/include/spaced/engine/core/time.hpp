#pragma once
#include <chrono>

using namespace std::chrono_literals;

namespace spaced {
using Clock = std::chrono::steady_clock;
using Duration = std::chrono::duration<float>;

///
/// \brief Stateful delta-time computer.
///
struct DeltaTime {
	///
	/// \brief Time of last update.
	///
	Clock::time_point start{Clock::now()};
	///
	/// \brief Cached value.
	///
	Duration value{};

	///
	/// \brief Update start time and obtain delta time.
	/// \returns Current delta time
	///
	auto operator()() -> Duration {
		auto const now = Clock::now();
		value = now - start;
		start = now;
		return value;
	}
};
} // namespace spaced
