#pragma once
#include <array>
#include <concepts>
#include <string_view>

namespace le {
template <typename Type>
concept EnumT = std::is_enum_v<Type>;

template <EnumT E, typename Type, std::size_t Size = std::size_t(E::eCOUNT_)>
struct EnumArray {
	std::array<Type, Size> values{};

	[[nodiscard]] constexpr auto at(E e) const -> Type const& { return values[static_cast<std::size_t>(e)]; }
	[[nodiscard]] constexpr auto at(E e) -> Type& { return values[static_cast<std::size_t>(e)]; }

	constexpr auto operator[](E e) const -> Type const& { return at(e); }
	constexpr auto operator[](E e) -> Type& { return at(e); }
};

template <EnumT E, typename Type, std::size_t Size = std::size_t(E::eCOUNT_)>
class StaticEnumArray : public EnumArray<E, Type, Size> {
  public:
	template <std::convertible_to<Type>... Values>
		requires(sizeof...(Values) == Size)
	consteval StaticEnumArray(Values... values) : EnumArray<E, Type, Size>{.values = {values...}} {}
};

template <EnumT E, std::size_t Size = std::size_t(E::eCOUNT_)>
using EnumToString = EnumArray<E, std::string_view, Size>;

template <EnumT E, std::size_t Size = std::size_t(E::eCOUNT_)>
using StaticEnumToString = StaticEnumArray<E, std::string_view, Size>;
} // namespace le
