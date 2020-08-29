#pragma once
#include <chrono>
#include <core/std_types.hpp>
#include <core/fixed.hpp>

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
	static Time elapsed() noexcept;
	///
	/// \brief Obtain time elapsed since epoch
	///
	static Time sinceEpoch() noexcept;
	static Time clamp(Time val, Time min, Time max) noexcept;
	static void resetElapsed() noexcept;

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
	Time& scale(f32 magnitude) noexcept;
	///
	/// \brief Obtained scaled time
	///
	Time scaled(f32 magnitude) const noexcept;

	Time& operator-() noexcept;
	Time& operator+=(Time const& rhs) noexcept;
	Time& operator-=(Time const& rhs) noexcept;
	Time& operator*=(Time const& rhs) noexcept;
	Time& operator/=(Time const& rhs) noexcept;

	bool operator==(Time const& rhs) noexcept;
	bool operator!=(Time const& rhs) noexcept;
	bool operator<(Time const& rhs) noexcept;
	bool operator<=(Time const& rhs) noexcept;
	bool operator>(Time const& rhs) noexcept;
	bool operator>=(Time const& rhs) noexcept;

	///
	/// \brief Serialise to seconds
	///
	f32 to_s() const noexcept;
	///
	/// \brief Serialise to milliseconds
	///
	s32 to_ms() const noexcept;
	///
	/// \brief Serialise to microseconds
	///
	s64 to_us() const noexcept;
};

Time operator+(Time const& lhs, Time const& rhs) noexcept;
Time operator-(Time const& lhs, Time const& rhs) noexcept;
Time operator*(Time const& lhs, Time const& rhs) noexcept;
Time operator/(Time const& lhs, Time const& rhs) noexcept;
} // namespace le
