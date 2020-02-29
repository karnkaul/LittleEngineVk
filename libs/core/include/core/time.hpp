#pragma once
#include <chrono>
#include "core/std_types.hpp"
#include "core/fixed.hpp"

namespace le
{
namespace stdch = std::chrono;

struct Time
{
	stdch::microseconds usecs;

	static std::string toString(Time time);
	static Time from_us(s64 microSeconds);
	static Time from_ms(s32 milliSeconds);
	static Time from_s(f32 seconds);
	static Time elapsed();
	static Time sinceEpoch();
	static Time clamp(Time val, Time min, Time max);
	static void resetElapsed();

	constexpr Time() noexcept : usecs(0) {}
	explicit constexpr Time(s64 usecs) noexcept : usecs(usecs) {}
	explicit constexpr Time(stdch::microseconds usecs) noexcept : usecs(usecs) {}

	Time& scale(f32 magnitude);
	Time scaled(f32 magnitude) const;

	Time& operator-();
	Time& operator+=(Time const& rhs);
	Time& operator-=(Time const& rhs);
	Time& operator*=(Time const& rhs);
	Time& operator/=(Time const& rhs);

	bool operator==(Time const& rhs);
	bool operator!=(Time const& rhs);
	bool operator<(Time const& rhs);
	bool operator<=(Time const& rhs);
	bool operator>(Time const& rhs);
	bool operator>=(Time const& rhs);

	f32 to_s() const;
	s32 to_ms() const;
	s64 to_us() const;
};

Time operator+(Time const& lhs, Time const& rhs);
Time operator-(Time const& lhs, Time const& rhs);
Time operator*(Time const& lhs, Time const& rhs);
Time operator/(Time const& lhs, Time const& rhs);
} // namespace le
