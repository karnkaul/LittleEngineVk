#include <chrono>
#include <sstream>
#include <core/time.hpp>

namespace le
{
namespace
{
static auto s_localEpoch = stdch::steady_clock::now();
} // namespace

std::string Time::toString(Time time)
{
	std::stringstream ret;
	ret << "[";
	s32 h = s32(time.to_s() / 60 / 60);
	if (h > 0)
	{
		ret << h << ":";
	}
	s32 m = s32((time.to_s() / 60) - f32(h * 60));
	if (m > 0)
	{
		ret << m << ":";
	}
	f32 s = (f32(time.to_ms()) / 1000.0f) - f32(h * 60 * 60) - f32(m * 60);
	if (s > 0.0f)
	{
		ret << s;
	}
	ret << "]";
	return ret.str();
}

Time Time::elapsed() noexcept
{
	return Time(stdch::steady_clock::now() - s_localEpoch);
}

Time Time::sinceEpoch() noexcept
{
	return Time(stdch::steady_clock::time_point().time_since_epoch());
}

Time Time::clamp(Time val, Time min, Time max) noexcept
{
	if (val.usecs < min.usecs)
	{
		return min;
	}
	if (val.usecs > max.usecs)
	{
		return max;
	}
	return val;
}

void Time::resetElapsed() noexcept
{
	s_localEpoch = stdch::steady_clock::now();
}

Time& Time::scale(f32 magnitude) noexcept
{
	auto fus = f32(usecs.count()) * magnitude;
	usecs = stdch::microseconds(s64(fus));
	return *this;
}

Time Time::scaled(f32 magnitude) const noexcept
{
	Time ret = *this;
	return ret.scale(magnitude);
}

Time& Time::operator-() noexcept
{
	usecs = -usecs;
	return *this;
}

Time& Time::operator+=(Time const& rhs) noexcept
{
	usecs += rhs.usecs;
	return *this;
}

Time& Time::operator-=(Time const& rhs) noexcept
{
	usecs -= rhs.usecs;
	return *this;
}

Time& Time::operator*=(Time const& rhs) noexcept
{
	usecs *= rhs.usecs.count();
	return *this;
}

Time& Time::operator/=(Time const& rhs) noexcept
{
	usecs = (rhs.usecs == usecs.zero()) ? usecs.zero() : usecs /= rhs.usecs.count();
	return *this;
}

bool Time::operator==(Time const& rhs) noexcept
{
	return usecs == rhs.usecs;
}

bool Time::operator!=(Time const& rhs) noexcept
{
	return !(*this == rhs);
}

bool Time::operator<(Time const& rhs) noexcept
{
	return usecs < rhs.usecs;
}

bool Time::operator<=(Time const& rhs) noexcept
{
	return usecs <= rhs.usecs;
}

bool Time::operator>(Time const& rhs) noexcept
{
	return usecs > rhs.usecs;
}

bool Time::operator>=(Time const& rhs) noexcept
{
	return usecs >= rhs.usecs;
}

f32 Time::to_s() const noexcept
{
	auto const count = usecs.count();
	return count == 0 ? 0.0f : f32(usecs.count()) / (1000.0f * 1000.0f);
}

s32 Time::to_ms() const noexcept
{
	auto const count = usecs.count();
	return count == 0 ? 0 : s32(usecs.count() / 1000);
}

s64 Time::to_us() const noexcept
{
	return usecs.count();
}

Time operator+(Time const& lhs, Time const& rhs) noexcept
{
	return Time(lhs) -= rhs;
}

Time operator-(Time const& lhs, Time const& rhs) noexcept
{
	return Time(lhs) -= rhs;
}

Time operator*(Time const& lhs, Time const& rhs) noexcept
{
	return Time(lhs) *= rhs;
}

Time operator/(Time const& lhs, Time const& rhs) noexcept
{
	return Time(lhs) /= rhs;
}

} // namespace le
