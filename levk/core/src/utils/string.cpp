#include <ktl/async/kmutex.hpp>
#include <levk/core/utils/string.hpp>
#include <algorithm>
#include <cerrno>
#include <unordered_map>

#if defined(__GNUG__)
#include <cxxabi.h>
#endif

namespace le {
namespace {
struct TNames {
	using Full = std::string_view;
	struct Demangled {
		std::string full;
		std::string_view minimal;
	};

	ktl::strict_tmutex<std::unordered_map<Full, Demangled>> names;

	static std::string_view minimalName(std::string_view full) noexcept {
		static constexpr std::string_view prefix = "::";
		static constexpr std::string_view skip = "<";
		auto ps = full.find(prefix);
		auto ss = full.find(skip);
		while (ps < full.size() && ss > ps) {
			full = full.substr(ps + prefix.size());
			ps = full.find(prefix);
			ss = full.find(skip);
		}
		return full;
	}

	static void build(Demangled& out, std::string_view name) {
#if defined(__GNUG__)
		int status = -1;
		std::size_t len = 0;
		char* szRes = abi::__cxa_demangle(name.data(), nullptr, &len, &status);
		if (status == 0 && szRes) {
			out.full = szRes;
			out.minimal = minimalName(out.full);
		}
		std::free(szRes);
#else
		static constexpr std::string_view CLASS = "class ";
		static constexpr std::string_view STRUCT = "struct ";
		auto idx = name.find(CLASS);
		if (idx == 0) { name = name.substr(CLASS.size()); }
		idx = name.find(STRUCT);
		if (idx == 0) { name = name.substr(STRUCT.size()); }
		out.full = name;
		out.minimal = minimalName(out.full);
#endif
	}

	std::string_view operator()(Full name, bool minimal) {
		if (name.empty()) { return {}; }
		auto lock = ktl::klock(names);
		auto it = lock->find(name);
		if (it == lock->end()) {
			auto [i, _] = lock->insert_or_assign(name, Demangled());
			it = i;
			build(it->second, name);
		}
		return minimal ? it->second.minimal : it->second.full;
	}

	void clear() noexcept { ktl::klock(names)->clear(); }
};

TNames g_tNames;
} // namespace

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

std::string_view utils::demangle(std::string_view name, bool minimal) { return g_tNames(name, minimal); }

std::string_view utils::removeNamespaces(std::string_view name) noexcept {
	auto const idx = name.find_last_of("::");
	if (idx != std::string::npos) { return name.substr(idx + 1); }
	return name;
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

std::string utils::toString(std::istream const& in) {
	std::stringstream str;
	str << in.rdbuf();
	return str.str();
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
