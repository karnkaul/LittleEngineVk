#include <sstream>
#include <fmt/format.h>
#include <core/gdata.hpp>
#include <core/utils.hpp>

namespace le
{
namespace
{
std::initializer_list<std::pair<char, char>> gDataEscapes = {{'{', '}'}, {'[', ']'}, {'"', '"'}};

template <typename T>
T get(std::unordered_map<std::string, std::string> const& table, std::string const& key, T (*Adaptor)(std::string_view, T),
	  T const& defaultValue)
{
	auto search = table.find(key);
	if (search != table.end())
	{
		return Adaptor(search->second, defaultValue);
	}
	return defaultValue;
}
} // namespace

GData::GData(std::string serialised)
{
	if (!serialised.empty())
	{
		marshall(std::move(serialised));
	}
}

GData::GData() = default;
GData::GData(GData&&) = default;
GData& GData::operator=(GData&&) = default;
GData::GData(GData const& rhs) = default;
GData& GData::operator=(GData const&) = default;
GData::~GData() = default;

bool GData::marshall(std::string serialised)
{
	utils::strings::removeChars(serialised, {'\t', '\r', '\n'});
	utils::strings::trim(serialised, {' '});
	if (serialised[0] == '{' && serialised[serialised.size() - 1] == '}')
	{
		clear();
		std::string rawText = serialised.substr(1, serialised.size() - 2);
		std::vector<std::string> tokens = utils::strings::tokenise(rawText, ',', gDataEscapes);
		for (auto const& token : tokens)
		{
			auto [first, second] = utils::strings::bisect(token, ':');
			if (!second.empty() && !first.empty())
			{
				utils::strings::trim(first, {' '});
				utils::strings::trim(first, {'"'});
				if (first != " ")
				{
					utils::strings::trim(first, {' '});
				}
				utils::strings::trim(second, {' ', '"'});
				m_fieldMap.emplace(std::move(first), std::move(second));
			}
		}
		return true;
	}

	return false;
}

std::string GData::unmarshall() const
{
	std::string ret = "{";
	size_t slice = 0;
	for (auto [key, value] : m_fieldMap)
	{
		utils::strings::trim(value, {' '});
		auto space = value.find(' ');
		if (utils::strings::isCharEnclosedIn(value, space, {'"', '"'}))
		{
			value = fmt::format("\"{}\"", std::move(value));
		}
		ret = fmt::format("{}{}:{},", std::move(ret), std::move(key), std::move(value));
		slice = 1;
	}
	return ret.substr(0, ret.size() - slice) + '}';
}

void GData::clear()
{
	m_fieldMap.clear();
}

std::string GData::getString(std::string const& key, std::string defaultValue) const
{
	auto search = m_fieldMap.find(key);
	if (search != m_fieldMap.end())
	{
		return search->second;
	}
	return defaultValue;
}

bool GData::getBool(std::string const& key, bool defaultValue) const
{
	return get<bool>(m_fieldMap, key, &utils::strings::toBool, defaultValue);
}

s32 GData::getS32(std::string const& key, s32 defaultValue) const
{
	return get<s32>(m_fieldMap, key, &utils::strings::toS32, defaultValue);
}

f64 GData::getF64(std::string const& key, f64 defaultValue) const
{
	return get<f64>(m_fieldMap, key, &utils::strings::toF64, defaultValue);
}

GData GData::getGData(std::string const& key) const
{
	auto search = m_fieldMap.find(key);
	if (search != m_fieldMap.end())
	{
		return GData(search->second);
	}
	return {};
}

std::vector<GData> GData::getGDatas(std::string const& key) const
{
	std::vector<GData> ret;
	std::vector<std::string> rawStrings = getVecString(key);
	for (auto& rawString : rawStrings)
	{
		utils::strings::trim(rawString, {'"', ' '});
		ret.push_back(GData(std::move(rawString)));
	}
	return ret;
}

std::vector<std::string> GData::getVecString(std::string const& key) const
{
	auto search = m_fieldMap.find(key);
	if (search != m_fieldMap.end())
	{
		std::string value = search->second;
		if (value.size() > 2)
		{
			utils::strings::trim(value, {' '});
			if (value[0] == '[' && value[value.size() - 1] == ']')
			{
				value = value.substr(1, value.size() - 2);
				utils::strings::trim(value, {' '});
				std::vector<std::string> ret = utils::strings::tokenise(value, ',', gDataEscapes);
				for (auto& str : ret)
				{
					utils::strings::trim(str, {'"', ' '});
				}
				return ret;
			}
		}
	}
	return {};
}

std::unordered_map<std::string, std::string> const& GData::allFields() const
{
	return m_fieldMap;
}

bool GData::addField(std::string key, GData& gData)
{
	std::string value = gData.unmarshall();
	utils::strings::removeChars(value, {'\"'});
	return setString(std::move(key), std::move(value));
}

bool GData::setString(std::string key, std::string value)
{
	if (!key.empty() && !value.empty())
	{
		m_fieldMap[std::move(key)] = std::move(value);
		return true;
	}
	return false;
}

u32 GData::fieldCount() const
{
	return u32(m_fieldMap.size());
}

bool GData::contains(std::string const& id) const
{
	return m_fieldMap.find(id) != m_fieldMap.end();
}
} // namespace le
