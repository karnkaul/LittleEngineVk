#pragma once
#include <limits>
#include <random>
#include "std_types.hpp"
#include "time.hpp"

namespace le::maths
{
using Time = le::Time;

// Returns val E [min, max]
template <typename T>
T const& clamp(T const& val, T const& min, T const& max);

template <typename T>
T randomRange(T min, T max);

template <typename T>
T lerp(T const& min, T const& max, f32 alpha);

bool isNearlyEqual(f32 lhs, f32 rhs, f32 epsilon = std::numeric_limits<f32>::epsilon());

class RandomGen
{
public:
	std::pair<s32, s32> m_s32Range;

protected:
	std::mt19937 m_intGen;
	std::mt19937 m_realGen;
	std::uniform_int_distribution<s32> m_intDist;
	std::uniform_real_distribution<f32> m_realDist;

public:
	RandomGen(s32 minS32, s32 maxS32, f32 minF32 = 0.0f, f32 maxF32 = 1.0f) noexcept;
	virtual ~RandomGen();

public:
	void seed(s32 seed);
	s32 nextS32();
	f32 nextF32();
};

template <typename T>
T const& clamp(T const& val, T const& min, T const& max)
{
	return (val < min) ? min : (val > max) ? max : val;
}

template <typename T>
T randomRange(T, T)
{
	static_assert(alwaysFalse<T>, "Only s32 and f32 are supported as T!");
}

template <>
s32 randomRange<s32>(s32 min, s32 max);

template <>
f32 randomRange<f32>(f32 min, f32 max);

template <typename T>
T lerp(T const& min, T const& max, f32 alpha)
{
	return min == max ? max : alpha * max + (1.0f - alpha) * min;
}
} // namespace le::maths
