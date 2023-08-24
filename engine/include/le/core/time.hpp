#pragma once
#include <chrono>

using namespace std::chrono_literals;

namespace le {
using Clock = std::chrono::steady_clock;

template <typename PeriodT = std::ratio<1, 1>>
using FDuration = std::chrono::duration<float, PeriodT>;

using Duration = FDuration<>;

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
} // namespace le
