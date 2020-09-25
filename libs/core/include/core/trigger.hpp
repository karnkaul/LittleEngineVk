#pragma once
#include <utility>
#include <core/std_types.hpp>

namespace le
{
template <typename T, T Default = T{}>
struct TTrigger final
{
	using type = T;
	inline static constexpr T null = Default;

	T t = null;
	s16 ttl = 1;

	constexpr TTrigger() = default;
	constexpr TTrigger(T t, s16 ttl = 1) noexcept(noexcept(T{})) : t(t), ttl(ttl) {}

	constexpr T get() noexcept;
};

template <typename T, T Default>
constexpr T TTrigger<T, Default>::get() noexcept
{
	if (t != null && ttl > 0)
	{
		--ttl;
		return t;
	}
	return t = null;
}
} // namespace le
