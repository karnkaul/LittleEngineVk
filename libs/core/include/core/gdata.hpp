#pragma once
#include <string>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <vector>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Dumb wrapper for reading JSON strings
///
/// Supported:
/// 	1. arrays ([...]), objects ({...}), and values
/// 	2. escaped characters (\")
///
class GData final
{
private:
	using Span = std::pair<std::size_t, std::size_t>;

	std::string m_raw;
	std::unordered_map<std::string, Span> m_fields;

public:
	[[nodiscard]] bool read(std::string json);

	template <typename T = std::string>
	T get(std::string const& key) const;

	///
	/// \brief Obtain string value corresponding to `key`
	///
	std::string getString(std::string const& key) const;
	///
	/// \brief Obtain array value corresponding to `key` (must be enclosed in [])
	///
	std::vector<std::string> getArray(std::string const& key) const;
	///
	/// \brief Obtain array of GData per array value corresponding to `key`
	///
	std::vector<GData> getDataArray(std::string const& key) const;
	///
	/// \brief Obtain GData of value corresponding to `key` (must be enclosed in {})
	///
	GData getData(std::string const& key) const;
	///
	/// \brief Obtain `s32` value corresponding to `key`
	///
	s32 getS32(std::string const& key) const;
	///
	/// \brief Obtain `f64` value corresponding to `key`
	///
	f64 getF64(std::string const& key) const;
	///
	/// \brief Obtain `bool` value corresponding to `key`
	///
	bool getBool(std::string const& key) const;

	///
	/// \brief Obtain whether `key` was parsed and is present in map
	///
	bool contains(std::string const& key) const;
	///
	/// \brief Reset (as if default constructed)
	///
	void clear();
	///
	/// \brief Obtain the count of fields parsed and saved
	///
	std::size_t fieldCount() const;
	///
	/// \brief Obtain all fields
	///
	std::unordered_map<std::string, std::string> allFields() const;
	///
	/// \brief Obtain original raw string
	///
	std::string const& original() const;
};

template <typename T>
T GData::get(std::string const& key) const
{
	if constexpr (std::is_same_v<T, std::string>)
	{
		return static_cast<T>(getString(key));
	}
	else if constexpr (std::is_integral_v<T>)
	{
		return static_cast<T>(getS32(key));
	}
	else if constexpr (std::is_floating_point_v<T>)
	{
		return static_cast<T>(getF64(key));
	}
	else if constexpr (std::is_same_v<T, GData>)
	{
		return getData(key);
	}
	else if constexpr (std::is_same_v<T, std::vector<std::string>>)
	{
		return getArray(key);
	}
	else if constexpr (std::is_same_v<T, std::vector<GData>>)
	{
		return getDataArray(key);
	}
	static_assert(alwaysFalse<T>, "Invalid type!");
}
} // namespace le
