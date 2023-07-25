#pragma once
#include <chrono>
#include <format>
#include <memory>

namespace spaced::logger {
using Clock = std::chrono::system_clock;

inline constexpr auto error_v{'E'};
inline constexpr auto warn_v{'W'};
inline constexpr auto info_v{'I'};
inline constexpr auto debug_v{'D'};

auto print(std::string_view domain, std::string_view message, char level) -> void;

struct Logger {
	std::string_view domain{};

	template <typename... Args>
	auto error(std::format_string<Args...> fmt, Args&&... args) const {
		print(domain, std::format(fmt, std::forward<Args>(args)...), error_v);
	}

	template <typename... Args>
	auto warn(std::format_string<Args...> fmt, Args&&... args) const {
		print(domain, std::format(fmt, std::forward<Args>(args)...), warn_v);
	}

	template <typename... Args>
	auto info(std::format_string<Args...> fmt, Args&&... args) const {
		print(domain, std::format(fmt, std::forward<Args>(args)...), info_v);
	}

	template <typename... Args>
	auto debug(std::format_string<Args...> fmt, Args&&... args) const {
		print(domain, std::format(fmt, std::forward<Args>(args)...), debug_v);
	}
};

struct File;

auto log_to_file(std::string path = "spaced.log") -> std::shared_ptr<File>;

auto const g_log{Logger{"General"}};
} // namespace spaced::logger
