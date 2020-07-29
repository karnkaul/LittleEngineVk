#pragma once
#include <functional>
#include <utility>

namespace le
{
///
/// \brief Struct to encapsulate zero-initialised `T` (primitive type)
///
template <typename T, T Zero = 0>
struct TZero final
{
	using type = T;

	constexpr static T s_null = Zero;

	T payload;

	constexpr TZero(T payload = s_null) noexcept : payload(payload) {}

	TZero(TZero&& rhs) noexcept
	{
		*this = std::move(rhs);
	}

	TZero& operator=(TZero&& rhs) noexcept
	{
		payload = std::forward<T>(rhs.payload);
		rhs.payload = Zero;
		return *this;
	}

	TZero(TZero const&) noexcept = default;
	TZero& operator=(TZero const&) noexcept = default;
	~TZero() = default;

	operator T() const noexcept
	{
		return payload;
	}
};
} // namespace le

namespace std
{
template <typename T, T Zero>
struct hash<le::TZero<T, Zero>>
{
	size_t operator()(le::TZero<T, Zero> zero) const noexcept
	{
		return std::hash<T>()(zero.payload);
	}
};
} // namespace std
