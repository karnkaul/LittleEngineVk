#include <algorithm>
#include <cerrno>
#include <core/utils/string.hpp>

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
	if (status == 0) { ret = szRes; }
	std::free(szRes);
#else
	static constexpr std::string_view CLASS = "class ";
	static constexpr std::string_view STRUCT = "struct ";
	auto idx = ret.find(CLASS);
	if (idx == 0) { ret = ret.substr(CLASS.size()); }
	idx = ret.find(STRUCT);
	if (idx == 0) { ret = ret.substr(STRUCT.size()); }
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
	if (idx != std::string::npos) { out_name = out_name.substr(idx + 1); }
}

void utils::toLower(std::string& outString) {
	std::transform(outString.begin(), outString.end(), outString.begin(), ::tolower);
	return;
}

void utils::toUpper(std::string& outString) {
	std::transform(outString.begin(), outString.end(), outString.begin(), ::toupper);
	return;
}

bool utils::toBool(std::string_view input, bool fallback) noexcept {
	if (!input.empty()) {
		if (input == "true" || input == "True" || input == "1") { return true; }
		if (input == "false" || input == "False" || input == "0") { return false; }
	}
	return fallback;
}

s64 utils::toS64(std::string_view input, s64 fallback) noexcept {
	s64 ret = fallback;
	if (!input.empty()) {
		try {
			ret = static_cast<s64>(std::atoll(input.data()));
		} catch (std::invalid_argument const&) { ret = fallback; }
	}
	return ret;
}

u64 utils::toU64(std::string_view input, u64 fallback) noexcept {
	u64 ret = fallback;
	if (!input.empty()) {
		try {
			ret = static_cast<u64>(std::stoull(input.data()));
		} catch (std::invalid_argument const&) { ret = fallback; }
		if (errno == ERANGE) {
			errno = 0;
			ret = fallback;
		}
	}
	return ret;
}

f64 utils::toF64(std::string_view input, f64 fallback) noexcept {
	f64 ret = fallback;
	if (!input.empty()) {
		try {
			ret = static_cast<f64>(std::stod(input.data()));
		} catch (std::invalid_argument const&) { ret = fallback; }
	}
	return ret;
}

std::string utils::toText(bytearray buffer) {
	std::vector<char> chars(buffer.size() + 1, 0);
	for (std::size_t i = 0; i < buffer.size(); ++i) { chars[i] = static_cast<char>(buffer[i]); }
	return std::string(chars.data());
}

std::pair<std::string, std::string> utils::bisect(std::string_view input, char delimiter) {
	std::size_t idx = input.find(delimiter);
	return idx < input.size() ? std::pair<std::string, std::string>(input.substr(0, idx), input.substr(idx + 1, input.size()))
							  : std::pair<std::string, std::string>(std::string(input), {});
}

void utils::substitute(std::string& outInput, std::initializer_list<std::pair<char, char>> replacements) {
	for (std::string::iterator iter = outInput.begin(); iter != outInput.end(); ++iter) {
		for (auto const& replacement : replacements) {
			if (*iter == replacement.first) {
				*iter = replacement.second;
				break;
			}
		}
	}
}
} // namespace le
