#pragma once
#include <filesystem>
#include <functional>
#include <type_traits>
#include <core/std_types.hpp>

namespace le
{
namespace stdfs = std::filesystem;

///
/// \brief Wrapper struct for storing the hash (via `std::hash`) for common types
///
struct Hash final
{
	std::size_t hash = 0;

	constexpr Hash() noexcept = default;

	///
	/// \brief Handled types: `char const*`, `std::string`, `std::filesystem::path`
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
	constexpr operator std::size_t() const noexcept
	{
		return hash;
	}
	///
	/// \brief Comparison operator
	///
	constexpr bool operator==(Hash rhs) const noexcept
	{
		return hash == rhs.hash;
	}
};

template <typename T>
Hash::Hash(T const& t)
{
	if constexpr (std::is_same_v<T, char*> || std::is_same_v<T, char const*> || std::is_same_v<T, std::string>)
	{
		hash = std::hash<std::string_view>{}(std::string_view(t));
	}
	else if constexpr (std::is_same_v<T, stdfs::path>)
	{
		hash = std::hash<std::string>{}(t.generic_string());
	}
	else
	{
		hash = std::hash<T>{}(t);
	}
}

template <std::size_t N>
Hash::Hash(char const (&t)[N])
{
	hash = std::hash<std::string_view>{}(std::string_view(t));
}

inline constexpr bool operator!=(Hash lhs, Hash rhs) noexcept
{
	return !(lhs == rhs);
}
} // namespace le

namespace std
{
template <>
struct hash<le::Hash>
{
	size_t operator()(le::Hash hash) const noexcept
	{
		return hash;
	}
};
} // namespace std
