#pragma once
#include <chrono>

using namespace std::chrono_literals;

namespace levk {
using Clock = std::chrono::steady_clock;
using Seconds = std::chrono::duration<float>;

/// \brief Stateful delta-time computer.
struct DeltaTime {
	/// \brief Time of last update.
	Clock::time_point start{Clock::now()};
	/// \brief Cached value.
	Seconds value{};

	/// \brief Update start time and obtain delta time.
	/// \returns Current delta time
	auto operator()() -> Seconds {
		auto const now = Clock::now();
		value = now - start;
		start = now;
		return value;
	}
};
} // namespace levk
