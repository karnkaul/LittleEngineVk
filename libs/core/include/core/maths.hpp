#pragma once
#include <cmath>
#include <limits>
#include <optional>
#include <random>
#include <type_traits>
#include <core/std_types.hpp>

namespace le::maths {
///
/// \brief Obtain the max value for `T`
///
template <typename T>
constexpr T max() noexcept;

///
/// \brief Obtain the epsilon value for `T`
///
template <typename T>
constexpr T epsilon() noexcept;

///
/// \brief Obtain the absolute value of `val`
///
template <typename T>
constexpr T abs(T const& val) noexcept;

///
/// \brief Check whether `abs(lhs - rhs)` is less than `e`
///
template <typename T>
constexpr bool equals(T const& lhs, T const& rhs, T const& e = epsilon<T>()) noexcept;

///
/// \brief Check whether `value` is within `min` and `max`
///
template <typename T>
constexpr bool inRange(T const& value, T const& min, T const& max, bool bInclusive = true) noexcept;

///
/// \brief Linearly interpolate between `min` to `max` based on `alpha`
///
template <typename T>
constexpr T lerp(T const& min, T const& max, f32 alpha) noexcept;

///
/// \brief Obtain a random number between min and max
///
template <typename T>
T randomRange(T min, T max) noexcept;

///
/// \brief Random Number Generator
///
class Random {
  public:
	///
	/// \brief Recreate and seed the engine with `value`
	///
	void seed(std::optional<u32> value = {}) noexcept;

	///
	/// \brief Obtain a random number given `Dist` distrubtion
	///
	template <typename T, template <typename> typename Dist, typename... Args>
	T range(Args&&... args) noexcept;

	///
	/// \brief Obtain an integral/floating point random number using a uniform distribution
	///
	template <typename T>
	T range(T min, T max) noexcept;

  protected:
	std::random_device m_device;
	std::mt19937 m_engine;
};

template <typename T>
constexpr T max() noexcept {
	static_assert(std::is_arithmetic_v<T>, "T must be arithemtic!");
	return std::numeric_limits<T>::max();
}

template <typename T>
constexpr T epsilon() noexcept {
	static_assert(std::is_arithmetic_v<T>, "T must be arithemtic!");
	return std::numeric_limits<T>::epsilon();
}

template <typename T>
constexpr T abs(T const& val) noexcept {
	return val < 0 ? -val : val;
}

template <typename T>
constexpr bool equals(T const& lhs, T const& rhs, T const& epsilon) noexcept {
	return std::abs(lhs - rhs) < epsilon;
}

template <typename T>
constexpr T lerp(T const& min, T const& max, f32 alpha) noexcept {
	return min == max ? max : alpha * max + (1.0f - alpha) * min;
}

template <typename T>
constexpr bool inRange(T const& value, T const& min, T const& max, bool bInclusive) noexcept {
	return bInclusive ? value >= min && value <= max : value > min && value < max;
}

template <typename T>
T randomRange(T min, T max) noexcept {
	static Random s_random;
	static bool s_bSeeded = false;
	if (!s_bSeeded) {
		s_random.seed();
		s_bSeeded = true;
	}
	return s_random.template range<T>(min, max);
}

inline void Random::seed(std::optional<u32> value) noexcept { m_engine = std::mt19937(value.value_or(m_device())); }

template <typename T, template <typename> typename Dist, typename... Args>
T Random::range(Args&&... args) noexcept {
	Dist<T> dist(std::forward<Args>(args)...);
	return dist(m_engine);
}

template <typename T>
T Random::range(T min, T max) noexcept {
	if constexpr (std::is_integral_v<T>) {
		return range<T, std::uniform_int_distribution>(min, max);
	} else if constexpr (std::is_floating_point_v<T>) {
		return range<T, std::uniform_real_distribution>(min, max);
	} else {
		static_assert(false_v<T>, "Invalid type!");
	}
}
} // namespace le::maths
