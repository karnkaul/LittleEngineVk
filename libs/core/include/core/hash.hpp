#pragma once
#include <functional>
#include <type_traits>
#include <core/io/path.hpp>
#include <core/std_types.hpp>

namespace le {
///
/// \brief Wrapper struct for storing the hash (via `std::hash`) for common types
///
struct Hash final {
	std::size_t hash = 0;

	constexpr Hash() noexcept = default;

	///
	/// \brief Handled types: `char const*`, `std::string`, io::Path
	///
	template <typename T>
	Hash(T const& t);
	///
	/// \brief Handled types: string literals
	///
	template <std::size_t N>
	Hash(char const (&t)[N]);

	///
	/// \brief Implicit conversion to `std::std::size_t`
	///
	constexpr operator std::size_t() const noexcept;
};

///
/// \brief Comparison operator
///
inline constexpr bool operator==(Hash lhs, Hash rhs) noexcept { return lhs.hash == rhs.hash; }
///
/// \brief Comparison operator
///
inline constexpr bool operator!=(Hash lhs, Hash rhs) noexcept { return !(lhs == rhs); }

template <typename T>
Hash::Hash(T const& t) {
	if constexpr (std::is_convertible_v<T, std::string_view>) {
		auto const str = std::string_view(t);
		hash = str.empty() ? 0 : std::hash<std::string_view>{}(str);
	} else if constexpr (std::is_same_v<T, io::Path>) {
		auto const str = t.generic_string();
		hash = str.empty() ? 0 : std::hash<std::string>{}(str);
	} else {
		hash = std::hash<T>{}(t);
	}
}

template <std::size_t N>
Hash::Hash(char const (&t)[N]) {
	hash = std::hash<std::string_view>{}(std::string_view(t));
}
constexpr inline Hash::operator std::size_t() const noexcept { return hash; }
} // namespace le

namespace std {
template <>
struct hash<le::Hash> {
	size_t operator()(le::Hash hash) const noexcept { return hash; }
};
} // namespace std
