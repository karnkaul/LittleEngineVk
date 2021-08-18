#pragma once
#include <functional>
#include <type_traits>
#include <core/std_types.hpp>
#include <core/utils/std_hash.hpp>

namespace le {
template <typename T>
concept hashable_t = requires(T const& t) {
	{ std::hash<T>{}(t) } -> std::convertible_to<std::size_t>;
};

///
/// \brief Wrapper struct for storing the hash (via std::hash) for common types
///
struct Hash final {
	std::size_t hash = 0;

	template <hashable_t T>
	static std::size_t compute(T const& t) noexcept;

	constexpr Hash() noexcept = default;

	///
	/// \brief Comparison operator
	///
	constexpr bool operator==(Hash const&) const = default;

	///
	/// \brief Handled types: char const*, std::string, io::Path
	///
	template <typename T>
	Hash(T const& t) noexcept : hash(compute(t)) {}
	///
	/// \brief Handled types: string literals
	///
	template <std::size_t N>
	Hash(char const (&str)[N]) noexcept : hash(std::hash<std::string_view>{}(str)) {}

	///
	/// \brief Implicit conversion to std::std::size_t
	///
	constexpr operator std::size_t() const noexcept { return hash; }
};

inline Hash operator"" _h(char const* str, std::size_t size) noexcept { return Hash(std::string_view(str, size)); }

// impl

template <hashable_t T>
std::size_t Hash::compute(T const& t) noexcept {
	if constexpr (std::is_convertible_v<T, std::string_view>) {
		if constexpr (std::is_null_pointer_v<T>) {
			return 0;
		} else {
			auto const str = std::string_view(t);
			return str.empty() ? 0 : std::hash<std::string_view>{}(str);
		}
	} else if constexpr (std::is_same_v<T, io::Path>) {
		auto const str = t.generic_string();
		return str.empty() ? 0 : std::hash<std::string>{}(str);
	} else {
		return std::hash<T>{}(t);
	}
}
} // namespace le

namespace std {
template <>
struct hash<le::Hash> {
	size_t operator()(le::Hash hash) const noexcept { return hash; }
};
} // namespace std
