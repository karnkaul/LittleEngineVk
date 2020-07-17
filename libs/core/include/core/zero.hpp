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
	TZero(TZero&&) noexcept;
	TZero& operator=(TZero&&) noexcept;
	TZero(TZero const&);
	TZero& operator=(TZero const&);
	~TZero();

	operator T() const;
};

template <typename T, T Zero>
TZero<T, Zero>::TZero(TZero&& rhs) noexcept
{
	*this = std::move(rhs);
}

template <typename T, T Zero>
TZero<T, Zero>& TZero<T, Zero>::operator=(TZero&& rhs) noexcept
{
	payload = rhs.payload;
	rhs.payload = Zero;
	return *this;
}

template <typename T, T Zero>
TZero<T, Zero>::TZero(TZero const& rhs) = default;

template <typename T, T Zero>
TZero<T, Zero>& TZero<T, Zero>::operator=(TZero const& rhs) = default;

template <typename T, T Zero>
TZero<T, Zero>::~TZero() = default;

template <typename T, T Zero>
TZero<T, Zero>::operator T() const
{
	return payload;
}
} // namespace le

namespace std
{
template <typename T, T Zero>
struct hash<le::TZero<T, Zero>>
{
	size_t operator()(le::TZero<T, Zero> zero) const
	{
		return std::hash<T>()(zero.payload);
	}
};
} // namespace std
