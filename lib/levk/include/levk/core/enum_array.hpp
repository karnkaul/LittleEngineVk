#pragma once
#include <levk/core/enum_t.hpp>
#include <array>

namespace levk {
/// \brief Enum-indexed wrapper around std::array.
template <EnumT E, typename Type, std::size_t Size = std::size_t(E::eCOUNT_)>
class EnumArray : public std::array<Type, Size> {
  public:
	EnumArray() = default;

	/// \brief Constructor to initialize all array values.
	/// \param t values to set.
	template <std::convertible_to<Type>... T>
		requires(sizeof...(T) == Size)
	constexpr EnumArray(T... t) : std::array<Type, Size>{std::move(t)...} {}

	[[nodiscard]] constexpr auto at(E const e) -> decltype(auto) { return std::array<Type, Size>::at(static_cast<std::size_t>(e)); }
	[[nodiscard]] constexpr auto at(E const e) const -> decltype(auto) { return std::array<Type, Size>::at(static_cast<std::size_t>(e)); }

	[[nodiscard]] constexpr auto operator[](E const e) -> decltype(auto) { return std::array<Type, Size>::operator[](static_cast<std::size_t>(e)); }
	[[nodiscard]] constexpr auto operator[](E const e) const -> decltype(auto) { return std::array<Type, Size>::operator[](static_cast<std::size_t>(e)); }
};
} // namespace levk
