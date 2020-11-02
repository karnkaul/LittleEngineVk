#include <algorithm>
#include <array>
#include <cstring>
#include <stack>
#include <core/utils.hpp>
#if defined(__GNUG__)
#include <cxxabi.h>
#endif

namespace le {
std::pair<f32, std::string_view> utils::friendlySize(u64 byteCount) noexcept {
	static constexpr std::array suffixes = {"B"sv, "KiB"sv, "MiB"sv, "GiB"sv};
	f32 bytes = f32(byteCount);
	std::size_t idx = 0;
	while (bytes > 1024.0f && idx < 4) {
		++idx;
		bytes /= 1024.0f;
	}
	return {bytes, suffixes[idx < 4 ? idx : 3]};
}

std::string utils::demangle(std::string_view name, bool bMinimal) {
	std::string ret(name);
#if defined(__GNUG__)
	s32 status = -1;
	char* szRes = abi::__cxa_demangle(name.data(), nullptr, nullptr, &status);
	if (status == 0) {
		ret = szRes;
	}
	std::free(szRes);
#else
	static constexpr std::string_view CLASS = "class ";
	static constexpr std::string_view STRUCT = "struct ";
	auto idx = ret.find(CLASS);
	if (idx == 0) {
		ret = ret.substr(CLASS.size());
	}
	idx = ret.find(STRUCT);
	if (idx == 0) {
		ret = ret.substr(STRUCT.size());
	}
#endif
	if (bMinimal) {
		static constexpr std::string_view prefix = "::";
		static constexpr std::string_view skip = "<";
		auto ps = ret.find(prefix);
		auto ss = ret.find(skip);
		while (ps < ret.size() && ss > ps) {
			ret = ret.substr(ps + prefix.size());
			ps = ret.find(prefix);
			ss = ret.find(skip);
		}
		return ret;
	}
	return ret;
}

void utils::removeNamesapces(std::string& out_name) {
	auto const idx = out_name.find_last_of("::");
	if (idx != std::string::npos) {
		out_name = out_name.substr(idx + 1);
	}
}

namespace utils {
void strings::toLower(std::string& outString) {
	std::transform(outString.begin(), outString.end(), outString.begin(), ::tolower);
	return;
}

void strings::toUpper(std::string& outString) {
	std::transform(outString.begin(), outString.end(), outString.begin(), ::toupper);
	return;
}

bool strings::toBool(std::string_view input, bool bDefaultValue) {
	if (!input.empty()) {
		if (input == "true" || input == "True" || input == "1") {
			return true;
		}
		if (input == "false" || input == "False" || input == "0") {
			return false;
		}
	}
	return bDefaultValue;
}

s32 strings::toS32(std::string_view input, s32 defaultValue) {
	s32 ret = defaultValue;
	if (!input.empty()) {
		try {
			ret = std::atoi(input.data());
		} catch (std::invalid_argument const&) {
			ret = defaultValue;
		}
	}
	return ret;
}

f32 strings::toF32(std::string_view input, f32 defaultValue) {
	f32 ret = defaultValue;
	if (!input.empty()) {
		try {
			ret = (f32)std::atof(input.data());
		} catch (std::invalid_argument const&) {
			ret = defaultValue;
		}
	}
	return ret;
}

f64 strings::toF64(std::string_view input, f64 defaultValue) {
	f64 ret = defaultValue;
	if (!input.empty()) {
		try {
			ret = std::atof(input.data());
		} catch (std::invalid_argument const&) {
			ret = defaultValue;
		}
	}
	return ret;
}

std::string strings::toText(bytearray rawBuffer) {
	std::vector<char> charBuffer(rawBuffer.size() + 1, 0);
	for (std::size_t i = 0; i < rawBuffer.size(); ++i) {
		charBuffer[i] = static_cast<char>(rawBuffer[i]);
	}
	return std::string(charBuffer.data());
}

std::pair<std::string, std::string> strings::bisect(std::string_view input, char delimiter) {
	std::size_t idx = input.find(delimiter);
	return idx < input.size() ? std::pair<std::string, std::string>(input.substr(0, idx), input.substr(idx + 1, input.size()))
							  : std::pair<std::string, std::string>(std::string(input), {});
}

void strings::removeChars(std::string& outInput, std::initializer_list<char> toRemove) {
	auto isToRemove = [&toRemove](char c) -> bool { return std::find(toRemove.begin(), toRemove.end(), c) != toRemove.end(); };
	auto iter = std::remove_if(outInput.begin(), outInput.end(), isToRemove);
	outInput.erase(iter, outInput.end());
	return;
}

void strings::trim(std::string& outInput, std::initializer_list<char> toRemove) {
	auto isIgnored = [&outInput, &toRemove](std::size_t idx) { return std::find(toRemove.begin(), toRemove.end(), outInput[idx]) != toRemove.end(); };
	std::size_t startIdx = 0;
	for (; startIdx < outInput.size() && isIgnored(startIdx); ++startIdx)
		;
	std::size_t endIdx = outInput.size();
	for (; endIdx > startIdx && isIgnored(endIdx - 1); --endIdx)
		;
	outInput = outInput.substr(startIdx, endIdx - startIdx);
	return;
}

void strings::removeWhitespace(std::string& outInput) {
	substituteChars(outInput, {std::pair<char, char>('\t', ' '), std::pair<char, char>('\n', ' '), std::pair<char, char>('\r', ' ')});
	removeChars(outInput, {' '});
	return;
}

std::vector<std::string> strings::tokenise(std::string_view s, char delimiter, std::initializer_list<std::pair<char, char>> escape) {
	auto end = s.cend();
	auto start = end;

	std::stack<std::pair<char, char>> escapeStack;
	std::vector<std::string> v;
	bool bEscaping = false;
	bool bSkipThis = false;
	for (auto it = s.cbegin(); it != end; ++it) {
		if (bSkipThis) {
			bSkipThis = false;
			continue;
		}
		bSkipThis = bEscaping && *it == '\\';
		if (bSkipThis) {
			continue;
		}
		if (*it != delimiter || bEscaping) {
			if (start == end) {
				start = it;
			}
			for (auto e : escape) {
				if (bEscaping && *it == e.second) {
					if (e.first == escapeStack.top().first) {
						escapeStack.pop();
						bEscaping = !escapeStack.empty();
						break;
					}
				}
				if (*it == e.first) {
					bEscaping = true;
					escapeStack.push(e);
					break;
				}
			}
			bSkipThis = false;
			continue;
		}
		if (start != end) {
			v.push_back({start, it});
			start = end;
		}
		bSkipThis = false;
	}
	if (start != end) {
		v.push_back({start, end});
	}
	return v;
}

std::vector<std::string_view> strings::tokeniseInPlace(char* szOutBuf, char delimiter, std::initializer_list<std::pair<char, char>> escape) {
	if (!szOutBuf || *szOutBuf == '\0') {
		return {};
	}

	char const* const end = szOutBuf + std::strlen(szOutBuf);
	char const* start = szOutBuf + std::strlen(szOutBuf);
	std::stack<std::pair<char, char>> escapeStack;
	std::vector<std::string_view> v;
	bool bEscaping = false;
	bool bSkipThis = false;
	for (char* it = szOutBuf; it != end; ++it) {
		if (bSkipThis) {
			bSkipThis = false;
			continue;
		}
		bSkipThis = bEscaping && *it == '\\';
		if (bSkipThis) {
			continue;
		}
		if (*it != delimiter || bEscaping) {
			if (start == end) {
				start = it;
			}
			for (auto e : escape) {
				if (bEscaping && *it == e.second) {
					if (e.first == escapeStack.top().first) {
						escapeStack.pop();
						bEscaping = !escapeStack.empty();
						break;
					}
				}
				if (*it == e.first) {
					bEscaping = true;
					escapeStack.push(e);
					break;
				}
			}
			bSkipThis = false;
			continue;
		}
		if (start != end) {
			*it = '\0';
			v.push_back(std::string_view(start));
			start = end;
		}
		bSkipThis = false;
	}
	if (start != end) {
		v.push_back(std::string_view(start));
	}
	return v;
}

void strings::substituteChars(std::string& outInput, std::initializer_list<std::pair<char, char>> replacements) {
	std::string::iterator iter = outInput.begin();
	while (iter != outInput.end()) {
		for (auto const& replacement : replacements) {
			if (*iter == replacement.first) {
				*iter = replacement.second;
				break;
			}
		}
		++iter;
	}
	return;
}

bool strings::isCharEnclosedIn(std::string_view str, std::size_t idx, std::pair<char, char> wrapper) {
	std::size_t idx_1 = idx - 1;
	std::size_t idx1 = idx + 1;
	return idx_1 < str.length() && idx1 < str.length() && str[idx_1] == wrapper.first && str[idx1] == wrapper.second;
}
} // namespace utils
} // namespace le
