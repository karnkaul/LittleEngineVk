#include <array>
#include <sstream>
#include <utility>
#include <core/log.hpp>
#include <core/gdata.hpp>
#include <core/utils.hpp>

namespace le
{
namespace
{
struct Escape final
{
	struct Sequence final
	{
		std::pair<char, char> match;
		s64 count = 0;
	};
	std::vector<Sequence> sequences;

	s64 stackSize(char c);
	void add(std::pair<char, char> match);
};

std::string sanitise(std::string_view str, size_t start, size_t end);
bool isWhitespace(char c);
bool isBoolean(std::string_view str, size_t first);

s64 Escape::stackSize(char c)
{
	s64 total = 0;
	for (auto& sequence : sequences)
	{
		if (c == sequence.match.first && sequence.match.first == sequence.match.second)
		{
			sequence.count = (sequence.count == 0) ? 1 : 0;
		}
		else
		{
			if (c == sequence.match.second)
			{
				ASSERT(sequence.count > 0, "Invalid escape sequence count!");
				--sequence.count;
			}
			else if (c == sequence.match.first)
			{
				++sequence.count;
			}
		}
		total += sequence.count;
	}
	return total;
}

void Escape::add(std::pair<char, char> match)
{
	sequences.push_back({match, 0});
}

std::string sanitise(std::string_view str, size_t begin = 0, size_t end = 0)
{
	std::string ret;
	ret.reserve(end - begin);
	bool bEscaped = false;
	if (end == 0)
	{
		end = str.size();
	}
	for (size_t idx = begin; idx < end; ++idx)
	{
		if (idx > 0 && str.at(idx) == '\\' && str.at(idx - 1) == '\\')
		{
			bEscaped = !bEscaped;
		}
		if (str.at(idx) != '\\' || bEscaped)
		{
			ret += str.at(idx);
		}
	}
	return ret;
}

bool isWhitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isBoolean(std::string_view str, size_t begin)
{
	static std::array<std::string_view, 2> const s_valid = {"true", "false"};
	if (begin < str.size())
	{
		for (auto const& valid : s_valid)
		{
			size_t const end = begin + valid.size();
			if (end < str.size() && std::string_view(str.data() + begin, end - begin) == valid)
			{
				return true;
			}
		}
	}
	return false;
}
} // namespace

bool GData::read(std::string json)
{
	m_raw = std::move(json);
	bool bStarted = false;
	u64 line = 1;
	for (size_t idx = 0; idx < m_raw.size();)
	{
		while (idx < m_raw.size() && isWhitespace(m_raw.at(idx)))
		{
			++idx;
		}
		if (idx >= m_raw.size())
		{
			break;
		}
		ASSERT(idx < m_raw.size(), "Invariant violated!");
		if (bStarted)
		{
			while (idx < m_raw.size() && (m_raw.at(idx) == '}' || isWhitespace(m_raw.at(idx))))
			{
				++idx;
			}
			if (idx == m_raw.size())
			{
				break;
			}
		}
		else
		{
			if (m_raw.at(idx) != '{')
			{
				LOG_E("[{}] Expected [{}] at index [{}] (line: {})", utils::tName<GData>(), idx, line);
				return false;
			}
			++idx;
		}
		bStarted = true;
		auto key = parseKey(idx, line);
		if (key.empty())
		{
			return false;
		}
		auto const [begin, end] = parseValue(idx, line);
		if (end <= begin)
		{
			return false;
		}
		if (m_fields.find(key) != m_fields.end())
		{
			LOG_W("[{}] [{}] Duplicate key! Overwriting value...", utils::tName<GData>(), key);
		}
		m_fields[key] = {begin, end};
	}
	if (m_fields.empty())
	{
		LOG_W("[{}] Empty json / nothing parsed", utils::tName<GData>());
		return false;
	}
	return true;
}

std::string GData::getString(std::string const& key) const
{
	if (auto search = m_fields.find(key); search != m_fields.end())
	{
		auto const [begin, end] = search->second;
		return sanitise(std::string_view(m_raw.data() + begin, end - begin));
	}
	return {};
}

std::vector<std::string> GData::getArray(std::string const& key) const
{
	std::vector<std::string> ret;
	if (auto search = m_fields.find(key); search != m_fields.end())
	{
		auto const [begin, end] = search->second;
		std::string_view value(m_raw.data() + begin, end - begin);
		if (value.size() > 2 && value.at(0) == '[' && value.at(value.size() - 1) == ']')
		{
			size_t idx = 1;
			Escape escape;
			escape.add({'[', ']'});
			escape.add({'{', '}'});
			escape.stackSize('[');
			while (idx < value.size())
			{
				while (idx < value.size() && isWhitespace(value.at(idx)))
				{
					++idx;
				}
				size_t first = idx;
				while (idx < value.size())
				{
					s64 const stack = escape.stackSize(value.at(idx));
					bool const bNext = stack <= 1 && value.at(idx) == ',';
					bool const bEnd = stack == 0 && value.at(idx) == ']';
					if (bNext || bEnd)
					{
						break;
					}
					++idx;
				}
				size_t last = idx >= value.size() ? value.size() - 1 : idx;
				if (value.at(last) == ']')
				{
					--last;
				}
				while (last > first && (value.at(last) == ',' || isWhitespace(value.at(last))))
				{
					--last;
				}
				if (last - first > 0)
				{
					if (value.at(first) == '\"')
					{
						ASSERT(value.at(last) == '\"', "Missing end quote!");
						++first;
						--last;
					}
					ret.push_back(sanitise(value, first, last + 1));
				}
				++idx;
			}
		}
	}
	return ret;
}

std::vector<GData> GData::getDataArray(std::string const& key) const
{
	std::vector<GData> ret;
	auto array = getArray(key);
	for (auto& str : array)
	{
		GData data;
		data.read(std::move(str));
		ret.push_back(std::move(data));
	}
	return ret;
}

GData GData::getData(std::string const& key) const
{
	GData ret;
	if (auto search = m_fields.find(key); search != m_fields.end())
	{
		auto const [begin, end] = search->second;
		ret.read(std::string(m_raw.data() + begin, end - begin));
	}
	return ret;
}

s32 GData::getS32(std::string const& key) const
{
	s32 ret = 0;
	if (auto search = m_fields.find(key); search != m_fields.end())
	{
		auto const [begin, end] = search->second;
		try
		{
			std::string_view value(m_raw.data() + begin, end - begin);
			std::array<char, 128> buffer;
			std::memcpy(buffer.data(), value.data(), value.size());
			buffer.at(value.size()) = '\0';
			ret = (s32)std::atoi(buffer.data());
		}
		catch (const std::exception& e)
		{
			LOG_E("[{}] Failed to parse [{}] into f32! {}", e.what());
		}
	}
	return ret;
}

f64 GData::getF64(std::string const& key) const
{
	f64 ret = 0;
	if (auto search = m_fields.find(key); search != m_fields.end())
	{
		auto const [begin, end] = search->second;
		try
		{
			std::string_view value(m_raw.data() + begin, end - begin);
			std::array<char, 128> buffer;
			std::memcpy(buffer.data(), value.data(), value.size());
			buffer.at(value.size()) = '\0';
			ret = (f64)std::atof(buffer.data());
		}
		catch (const std::exception& e)
		{
			LOG_E("[{}] Failed to parse [{}] into f32! {}", e.what());
		}
	}
	return ret;
}

bool GData::getBool(std::string const& key) const
{
	bool bRet = false;
	if (auto search = m_fields.find(key); search != m_fields.end())
	{
		auto const [begin, end] = search->second;
		std::string_view value(m_raw.data() + begin, end - begin);
		if (value == "1" || value == "true" || value == "TRUE")
		{
			bRet = true;
		}
	}
	return bRet;
}

bool GData::contains(std::string const& key) const
{
	return m_fields.find(key) != m_fields.end();
}

void GData::clear()
{
	m_fields.clear();
	m_raw.clear();
}

size_t GData::fieldCount() const
{
	return m_fields.size();
}

std::unordered_map<std::string, std::string> GData::allFields() const
{
	std::unordered_map<std::string, std::string> ret;
	for (auto const& [key, indices] : m_fields)
	{
		ret[key] = std::string(m_raw.data() + indices.first, indices.second - indices.first);
	}
	return ret;
}

std::string GData::parseKey(size_t& out_idx, u64& out_line)
{
	if (out_idx >= m_raw.size())
	{
		LOG_E("[{}] Unexpected end of string at index [{}] (line: {}), failed to extract key!", utils::tName<GData>(), out_idx, out_line);
		return {};
	}
	advance(out_idx, out_line);
	char c = m_raw.at(out_idx);
	if (c != '\"')
	{
		LOG_E("[{}] Expected: [\"] at index [{}] (line: {}), failed to extract key!", utils::tName<GData>(), out_idx, out_line);
		return {};
	}
	++out_idx;
	if (out_idx < m_raw.size() && m_raw.at(out_idx) == '\\')
	{
		++out_idx;
	}
	size_t const start = out_idx;
	++out_idx;
	if (out_idx >= m_raw.size())
	{
		LOG_E("[{}] Unexpected end of string at index [{}] (line: {}), failed to extract key!", utils::tName<GData>(), out_idx, out_line);
		return {};
	}
	while (out_idx < m_raw.size() && m_raw.at(out_idx) != '"')
	{
		++out_idx;
	}
	std::string ret = sanitise(m_raw, start, out_idx);
	++out_idx;
	advance(out_idx, out_line);
	if (out_idx >= m_raw.size() || m_raw.at(out_idx) != ':')
	{
		LOG_E("[{}] Expected [:] after key [{}] at index [{}] (line: {}), failed to extract key!", utils::tName<GData>(), ret, out_idx, out_line);
		return {};
	}
	++out_idx;
	advance(out_idx, out_line);
	return ret;
}

std::pair<size_t, size_t> GData::parseValue(size_t& out_idx, u64& out_line)
{
	if (out_idx >= m_raw.size())
	{
		LOG_E("[{}] Unexpected end of string at index [{}] (line: {}), failed to extract value!", utils::tName<GData>(), out_idx, out_line);
		return {};
	}
	advance(out_idx, out_line);
	char const c = m_raw.at(out_idx);
	bool const bQuoted = c == '\"';
	bool const bArray = !bQuoted && c == '[';
	bool const bObject = !bQuoted && !bArray && c == '{';
	bool const bBoolean = !bQuoted && !bArray && !bObject && isBoolean(m_raw, out_idx);
	bool const bNumeric = !bQuoted && !bArray && !bObject && !bBoolean;
	Escape escape;
	if (bQuoted)
	{
		escape.add({'\"', '\"'});
	}
	else if (bArray)
	{
		escape.add({'[', ']'});
	}
	else if (bObject)
	{
		escape.add({'{', '}'});
	}
	auto isEnd = [&](bool bNoStack = false) -> bool {
		char const x = m_raw.at(out_idx);
		s64 stack = 0;
		if (!bNoStack)
		{
			stack = escape.stackSize(x);
		}
		return stack == 0 && ((bQuoted && x == '\"') || (bArray && x == ']') || x == ',' || x == '}' || (bBoolean && isWhitespace(x)));
	};
	if (bQuoted)
	{
		++out_idx;
		escape.stackSize('\"');
	}
	if (out_idx >= m_raw.size())
	{
		LOG_E("[{}] Unexpected end of string at index [{}] (line: {}), failed to extract value!", utils::tName<GData>(), out_idx, out_line);
		return {};
	}
	advance(out_idx, out_line);
	size_t const begin = out_idx;
	bool const bNegativeOrNumeric = begin < m_raw.size() && (std::isdigit(m_raw.at(begin)) || m_raw.at(begin) == '-');
	while (out_idx < m_raw.size() && !isEnd())
	{
		if (!bQuoted)
		{
			if (bNumeric && !std::isdigit(m_raw.at(out_idx)) && m_raw.at(out_idx) != '.' && !bNegativeOrNumeric)
			{
				LOG_E("[{}] Expected [0-9] at index [{}] (line: {}), failed to extract value!", utils::tName<GData>(), out_idx, out_line);
				return {};
			}
		}
		if (m_raw.at(out_idx) == '\n')
		{
			++out_line;
		}
		++out_idx;
	}
	if (out_idx >= m_raw.size() || !isEnd(true))
	{
		char e = bQuoted ? '\"' : bArray ? ']' : '}';
		LOG_E("[{}] Expected [{}] at index [{}] (line: {}), failed to extract value!", utils::tName<GData>(), e, out_idx, out_line);
		return {};
	}
	if (bArray || bObject)
	{
		++out_idx;
	}
	size_t const end = out_idx;
	if (bQuoted)
	{
		++out_idx;
	}
	advance(out_idx, out_line);
	if (out_idx >= m_raw.size() || (m_raw.at(out_idx) != ',' && m_raw.at(out_idx) != '}'))
	{
		LOG_E("[{}] Unterminated value at index [{}] (line: {}), failed to extract value!", utils::tName<GData>(), out_idx, out_line);
		return {};
	}
	++out_idx;
	advance(out_idx, out_line);
	return {begin, end};
}

void GData::advance(size_t& out_idx, size_t& out_line) const
{
	while (out_idx < m_raw.size() && isWhitespace(m_raw.at(out_idx)))
	{
		if (m_raw.at(out_idx) == '\n')
		{
			++out_line;
		}
		++out_idx;
	}
}
} // namespace le
