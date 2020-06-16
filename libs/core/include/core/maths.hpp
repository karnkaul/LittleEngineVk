#pragma once
#include <cmath>
#include <limits>
#include <optional>
#include <random>
#include <type_traits>
#include <core/std_types.hpp>
#include <core/time.hpp>

namespace le::maths
{
///
/// \brief Obtain a random number between min and max
///
template <typename T>
T randomRange(T min, T max);

///
/// \brief Linearly interpolate between `min` to `max` based on `alpha`
///
template <typename T>
T lerp(T const& min, T const& max, f32 alpha);

///
/// \brief Check whether `abs(lhs - rhs)` is less than `epsilon`
///
template <typename T>
bool equals(T lhs, T rhs, T epsilon = std::numeric_limits<T>::epsilon());

///
/// \brief Random Number Generator
///
class Random
{
protected:
	std::random_device m_device;
	std::mt19937 m_engine;

public:
	///
	/// \brief Recreate and seed the engine with `value`
	///
	inline void seed(std::optional<u32> value = {})
	{
		m_engine = std::mt19937(value ? *value : m_device());
	}

	///
	/// \brief Obtain a random number given `Dist` distrubtion
	///
	template <typename T, template <typename> typename Dist, typename... Args>
	T inRange(Args... args);

	///
	/// \brief Obtain an integral/floating point random number using a uniform distribution
	///
	template <typename T>
	T inRange(T min, T max);
};

template <typename T, template <typename> typename Dist, typename... Args>
T Random::inRange(Args... args)
{
	Dist<T> dist(std::forward<Args>(args)...);
	return dist(m_engine);
}

template <typename T>
T Random::inRange(T min, T max)
{
	if constexpr (std::is_integral_v<T>)
	{
		return inRange<T, std::uniform_int_distribution>(min, max);
	}
	else if constexpr (std::is_floating_point_v<T>)
	{
		return inRange<T, std::uniform_real_distribution>(min, max);
	}
	else
	{
		static_assert(alwaysFalse<T>, "Invalid type!");
	}
}

template <typename T>
bool equals(T lhs, T rhs, T epsilon)
{
	return std::abs(lhs - rhs) < epsilon;
}

template <typename T>
T randomRange(T min, T max)
{
	static Random s_random;
	static bool s_bSeeded = false;
	if (!s_bSeeded)
	{
		s_random.seed();
		s_bSeeded = true;
	}
	return s_random.inRange<T>(min, max);
}

template <typename T>
T lerp(T const& min, T const& max, f32 alpha)
{
	return min == max ? max : alpha * max + (1.0f - alpha) * min;
}
} // namespace le::maths
