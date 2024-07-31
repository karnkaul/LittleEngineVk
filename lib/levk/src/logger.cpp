#include <levk/core/fixed_string.hpp>
#include <levk/logger.hpp>
#include <ctime>
#include <print>
#include <unordered_map>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace levk {
namespace {
[[nodiscard]] auto get_timestamp() {
	auto ret = std::array<char, 16>{};
	auto const time = std::time(nullptr);
	static auto mutex = std::mutex{};
	auto lock = std::scoped_lock{mutex};
	auto const* tm = std::localtime(&time); // NOLINT(concurrency-mt-unsafe)
	std::strftime(ret.data(), ret.size() - 1, "%T", tm);
	return ret;
}

struct {
	using Level = log::Level;

	std::unordered_map<std::string_view, Level> tag_levels{};
	Level max_level{Level::eDebug};
	mutable std::mutex mutex{};

	[[nodiscard]] auto get_max_level(std::string_view const tag) const {
		auto lock = std::scoped_lock{mutex};
		if (auto const it = tag_levels.find(tag); it != tag_levels.end()) { return it->second; }
		return max_level;
	}

	void set_max(Level const level) {
		auto lock = std::scoped_lock{mutex};
		max_level = level;
	}

	void set_max(std::string_view const tag, std::optional<Level> const level) {
		auto lock = std::scoped_lock{mutex};
		if (!level) {
			tag_levels.erase(tag);
		} else {
			tag_levels.insert_or_assign(tag, *level);
		}
	}

	void reset() {
		auto lock = std::scoped_lock{mutex};
		max_level = Level::eDebug;
		tag_levels.clear();
	}
} g_log_levels{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // namespace

auto log::format(thread::Index const thread_index, Level const level, std::string_view const tag, std::string_view const text) -> std::string {
	auto const tid = FixedString<4>{"T{}", static_cast<int>(thread_index)};
	auto const timestamp = get_timestamp();
	return std::format("[{}][{: >3}] [{}] {} [{}]\n", level_to_char_v[level], tid.as_view(), tag, text, timestamp.data());
}

void log::print(thread::Index const thread_index, Level const level, std::string_view const tag, std::string_view const text) {
	auto* out = level == Level::eError ? stderr : stdout;
	auto const message = format(thread_index, level, tag, text);
	std::print(out, "{}", message);

#if defined(_WIN32)
	OutputDebugStringA(message.c_str());
#endif
}

void log::set_max_level(Level const level) { g_log_levels.set_max(level); }
void log::set_max_level_for(std::string_view const tag, std::optional<Level> const level) { g_log_levels.set_max(tag, level); }
auto log::get_max_level(std::string_view const tag) -> Level { return g_log_levels.get_max_level(tag); }
void log::reset_max_levels() { g_log_levels.reset(); }
} // namespace levk
