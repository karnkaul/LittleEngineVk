#pragma once
#include <array>
#include <iterator>
#include <type_traits>

namespace le::utils {
///
/// \brief Iterator for contiguous enums
///
template <typename E>
	requires std::is_enum_v<E>
struct EnumIter {
	using value_type = E;
	using iterator_category = std::random_access_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using u_t = std::underlying_type_t<E>;

	E e;

	constexpr auto operator<=>(EnumIter const&) const = default;
	constexpr E operator*() const noexcept { return e; }
	constexpr EnumIter& operator++() noexcept { return ((e = E(u_t(e) + 1)), *this); }
	constexpr EnumIter& operator--() noexcept { return ((e = E(u_t(e) - 1)), *this); }
	constexpr EnumIter operator++(int) noexcept { return (++(*this), (*this) - 1); }
	constexpr EnumIter operator--(int) noexcept { return (--(*this), (*this) - 1); }
	constexpr EnumIter& operator+=(difference_type i) noexcept { return ((e = E(u_t(e) + u_t(i))), *this); }
	constexpr EnumIter& operator-=(difference_type i) noexcept { return ((e = E(u_t(e) - u_t(i))), *this); }
	constexpr EnumIter operator+(difference_type i) noexcept { return {E(u_t(e) + u_t(i))}; }
	constexpr EnumIter operator-(difference_type i) noexcept { return {E(u_t(e) - u_t(i))}; }
};

///
/// \brief Stateless, iteratable "container" of contiguous enum values
///
template <typename E, E Begin = E{}, E End = E::eCOUNT_>
	requires std::is_enum_v<E>
struct EnumValues {
	using value_type = E;
	using u_t = std::underlying_type_t<E>;
	using const_iterator = EnumIter<E>;

	inline static constexpr E begin_v = Begin;
	inline static constexpr E end_v = End;
	inline static constexpr std::size_t size_v = std::size_t(std::ptrdiff_t(End) - std::ptrdiff_t(Begin));

	static constexpr const_iterator begin() noexcept { return {Begin}; }
	static constexpr const_iterator end() noexcept { return {End}; }

	static constexpr std::array<E, size_v> values() noexcept {
		std::array<E, size_v> ret;
		for (std::size_t i = 0; i < size_v; ++i) { ret[i] = static_cast<E>(i); }
		return ret;
	}
};
} // namespace le::utils
