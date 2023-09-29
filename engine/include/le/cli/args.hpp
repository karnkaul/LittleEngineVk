#pragma once
#include <string_view>
#include <utility>

namespace le::cli {
constexpr std::string_view delimiters_v{" \t;"};

struct Args {
	std::string_view remain{};

	constexpr auto next(std::string_view& out) -> bool {
		if (is_empty()) { return false; }
		auto const i = remain.find_first_of(delimiters_v);
		if (i != std::string_view::npos) {
			out = remain.substr(0, i);
			remain = remain.substr(i + 1);
		} else {
			out = std::exchange(remain, {});
		}
		return true;
	}

	constexpr auto next() -> std::string_view {
		auto ret = std::string_view{};
		if (!next(ret)) { return {}; }
		return ret;
	}

	[[nodiscard]] constexpr auto peek() const -> std::string_view {
		auto copy = *this;
		return copy.next();
	}

	[[nodiscard]] constexpr auto is_empty() const -> bool { return remain.empty(); }
};
} // namespace le::cli
