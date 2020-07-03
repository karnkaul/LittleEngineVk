#pragma once
#include <string>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <vector>
#include <core/std_types.hpp>

namespace le
{
class GData final
{
private:
	std::string m_raw;
	std::unordered_map<std::string, std::pair<std::size_t, std::size_t>> m_fields;

public:
	[[nodiscard]] bool read(std::string json);

	template <typename T = std::string>
	T get(std::string const& key) const;

	std::string getString(std::string const& key) const;
	std::vector<std::string> getArray(std::string const& key) const;
	std::vector<GData> getDataArray(std::string const& key) const;
	GData getData(std::string const& key) const;
	s32 getS32(std::string const& key) const;
	f64 getF64(std::string const& key) const;
	bool getBool(std::string const& key) const;

	bool contains(std::string const& key) const;
	void clear();
	std::size_t fieldCount() const;
	std::unordered_map<std::string, std::string> allFields() const;

private:
	std::string parseKey(std::size_t& out_idx, u64& out_line);
	std::pair<std::size_t, std::size_t> parseValue(std::size_t& out_idx, u64& out_line);
	void advance(std::size_t& out_idx, std::size_t& out_line) const;
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
