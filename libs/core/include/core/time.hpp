#pragma once
#include <chrono>
#include <core/fixed.hpp>
#include <core/std_types.hpp>

namespace le {
namespace stdch = std::chrono;
using namespace std::chrono_literals;

template <typename Duration, typename Clock = std::chrono::steady_clock>
struct TimeSpan;

using Time = TimeSpan<stdch::microseconds>;

///
/// \brief Wrapper for a time span
///
template <typename Duration, typename Clock>
struct TimeSpan {
	inline static auto s_localEpoch = Clock::now();

	stdch::microseconds usecs;

	///
	/// \brief Obtain time elapsed since clock was reset (since epoch if never reset)
	///
	static TimeSpan elapsed() noexcept;
	///
	/// \brief Obtain time elapsed since epoch
	///
	static constexpr TimeSpan sinceEpoch() noexcept;
	static constexpr TimeSpan clamp(TimeSpan val, TimeSpan min, TimeSpan max) noexcept;
	static void resetElapsed() noexcept;

	constexpr TimeSpan() noexcept;
	template <typename Rep, typename Period>
	constexpr TimeSpan(stdch::duration<Rep, Period> duration) noexcept;
	explicit constexpr TimeSpan(s64 usecs) noexcept : usecs(usecs) {
	}
	explicit constexpr TimeSpan(stdch::microseconds usecs) noexcept : usecs(usecs) {
	}

	///
	/// \brief Scale time by `magnitude`
	///
	constexpr TimeSpan& scale(f32 magnitude) noexcept;
	///
	/// \brief Obtained scaled time
	///
	constexpr TimeSpan scaled(f32 magnitude) const noexcept;

	constexpr TimeSpan& operator-() noexcept;
	constexpr TimeSpan& operator+=(TimeSpan rhs) noexcept;
	constexpr TimeSpan& operator-=(TimeSpan rhs) noexcept;
	constexpr TimeSpan& operator*=(TimeSpan rhs) noexcept;
	constexpr TimeSpan& operator/=(TimeSpan rhs) noexcept;

	constexpr bool operator==(TimeSpan rhs) noexcept;
	constexpr bool operator!=(TimeSpan rhs) noexcept;
	constexpr bool operator<(TimeSpan rhs) noexcept;
	constexpr bool operator<=(TimeSpan rhs) noexcept;
	constexpr bool operator>(TimeSpan rhs) noexcept;
	constexpr bool operator>=(TimeSpan rhs) noexcept;

	///
	/// \brief Serialise to seconds
	///
	constexpr f32 to_s() const noexcept;
	///
	/// \brief Serialise to milliseconds
	///
	constexpr s32 to_ms() const noexcept;
	///
	/// \brief Serialise to microseconds
	///
	constexpr s64 to_us() const noexcept;
};

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> operator+(TimeSpan<Duration, Clock> const& lhs, TimeSpan<Duration, Clock> const& rhs) noexcept;
template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> operator-(TimeSpan<Duration, Clock> const& lhs, TimeSpan<Duration, Clock> const& rhs) noexcept;
template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> operator*(TimeSpan<Duration, Clock> const& lhs, TimeSpan<Duration, Clock> const& rhs) noexcept;
template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> operator/(TimeSpan<Duration, Clock> const& lhs, TimeSpan<Duration, Clock> const& rhs) noexcept;

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock>::TimeSpan() noexcept : usecs(0) {
}

template <typename Duration, typename Clock>
template <typename Rep, typename Period>
constexpr TimeSpan<Duration, Clock>::TimeSpan(stdch::duration<Rep, Period> duration) noexcept : usecs(stdch::duration_cast<Duration>(duration)) {
}

template <typename Duration, typename Clock>
TimeSpan<Duration, Clock> TimeSpan<Duration, Clock>::elapsed() noexcept {
	return TimeSpan<Duration, Clock>(stdch::steady_clock::now() - s_localEpoch);
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> TimeSpan<Duration, Clock>::sinceEpoch() noexcept {
	return TimeSpan(stdch::steady_clock::time_point().time_since_epoch());
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> TimeSpan<Duration, Clock>::clamp(TimeSpan val, TimeSpan min, TimeSpan max) noexcept {
	if (val.usecs < min.usecs) {
		return min;
	}
	if (val.usecs > max.usecs) {
		return max;
	}
	return val;
}

template <typename Duration, typename Clock>
void TimeSpan<Duration, Clock>::resetElapsed() noexcept {
	s_localEpoch = stdch::steady_clock::now();
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock>& TimeSpan<Duration, Clock>::scale(f32 magnitude) noexcept {
	auto fus = f32(usecs.count()) * magnitude;
	usecs = stdch::microseconds(s64(fus));
	return *this;
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> TimeSpan<Duration, Clock>::scaled(f32 magnitude) const noexcept {
	TimeSpan ret = *this;
	return ret.scale(magnitude);
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock>& TimeSpan<Duration, Clock>::operator-() noexcept {
	usecs = -usecs;
	return *this;
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock>& TimeSpan<Duration, Clock>::operator+=(TimeSpan rhs) noexcept {
	usecs += rhs.usecs;
	return *this;
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock>& TimeSpan<Duration, Clock>::operator-=(TimeSpan rhs) noexcept {
	usecs -= rhs.usecs;
	return *this;
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock>& TimeSpan<Duration, Clock>::operator*=(TimeSpan rhs) noexcept {
	usecs *= rhs.usecs.count();
	return *this;
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock>& TimeSpan<Duration, Clock>::operator/=(TimeSpan rhs) noexcept {
	usecs = (rhs.usecs == usecs.zero()) ? usecs.zero() : usecs /= rhs.usecs.count();
	return *this;
}

template <typename Duration, typename Clock>
constexpr bool TimeSpan<Duration, Clock>::operator==(TimeSpan rhs) noexcept {
	return usecs == rhs.usecs;
}

template <typename Duration, typename Clock>
constexpr bool TimeSpan<Duration, Clock>::operator!=(TimeSpan rhs) noexcept {
	return !(*this == rhs);
}

template <typename Duration, typename Clock>
constexpr bool TimeSpan<Duration, Clock>::operator<(TimeSpan rhs) noexcept {
	return usecs < rhs.usecs;
}

template <typename Duration, typename Clock>
constexpr bool TimeSpan<Duration, Clock>::operator<=(TimeSpan rhs) noexcept {
	return usecs <= rhs.usecs;
}

template <typename Duration, typename Clock>
constexpr bool TimeSpan<Duration, Clock>::operator>(TimeSpan rhs) noexcept {
	return usecs > rhs.usecs;
}

template <typename Duration, typename Clock>
constexpr bool TimeSpan<Duration, Clock>::operator>=(TimeSpan rhs) noexcept {
	return usecs >= rhs.usecs;
}

template <typename Duration, typename Clock>
constexpr f32 TimeSpan<Duration, Clock>::to_s() const noexcept {
	auto const count = usecs.count();
	return count == 0 ? 0.0f : f32(usecs.count()) / (1000.0f * 1000.0f);
}

template <typename Duration, typename Clock>
constexpr s32 TimeSpan<Duration, Clock>::to_ms() const noexcept {
	auto const count = usecs.count();
	return count == 0 ? 0 : s32(usecs.count() / 1000);
}

template <typename Duration, typename Clock>
constexpr s64 TimeSpan<Duration, Clock>::to_us() const noexcept {
	return usecs.count();
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> operator+(TimeSpan<Duration, Clock> const& lhs, TimeSpan<Duration, Clock> const& rhs) noexcept {
	return TimeSpan(lhs) -= rhs;
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> operator-(TimeSpan<Duration, Clock> const& lhs, TimeSpan<Duration, Clock> const& rhs) noexcept {
	return TimeSpan(lhs) -= rhs;
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> operator*(TimeSpan<Duration, Clock> const& lhs, TimeSpan<Duration, Clock> const& rhs) noexcept {
	return TimeSpan(lhs) *= rhs;
}

template <typename Duration, typename Clock>
constexpr TimeSpan<Duration, Clock> operator/(TimeSpan<Duration, Clock> const& lhs, TimeSpan<Duration, Clock> const& rhs) noexcept {
	return TimeSpan(lhs) /= rhs;
}
} // namespace le
