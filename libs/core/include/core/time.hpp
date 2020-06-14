#pragma once
#include <chrono>
#include "std_types.hpp"
#include "fixed.hpp"

namespace le
{
namespace stdch = std::chrono;
using namespace std::chrono_literals;

///
/// \brief Wrapper for a time span
///
/// Not a time point!
///
struct Time
{
	stdch::microseconds usecs;

	static std::string toString(Time time);
	///
	/// \brief Obtain time elapsed since clock was reset (since epoch if never reset)
	///
	static Time elapsed();
	///
	/// \brief Obtain time elapsed since epoch
	///
	static Time sinceEpoch();
	static Time clamp(Time val, Time min, Time max);
	static void resetElapsed();

	constexpr Time() noexcept : usecs(0) {}
	template <typename T>
	constexpr Time(stdch::duration<s64, T> duration) : usecs(stdch::duration_cast<stdch::microseconds>(duration))
	{
	}
	explicit constexpr Time(s64 usecs) noexcept : usecs(usecs) {}
	explicit constexpr Time(stdch::microseconds usecs) noexcept : usecs(usecs) {}

	///
	/// \brief Scale time by `magnitude`
	///
	Time& scale(f32 magnitude);
	///
	/// \brief Obtained scaled time
	///
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

	///
	/// \brief Serialise to seconds
	///
	f32 to_s() const;
	///
	/// \brief Serialise to milliseconds
	///
	s32 to_ms() const;
	///
	/// \brief Serialise to microseconds
	///
	s64 to_us() const;
};

Time operator+(Time const& lhs, Time const& rhs);
Time operator-(Time const& lhs, Time const& rhs);
Time operator*(Time const& lhs, Time const& rhs);
Time operator/(Time const& lhs, Time const& rhs);
} // namespace le
