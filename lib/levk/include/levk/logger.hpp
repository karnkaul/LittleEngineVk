#pragma once
#include <levk/core/enum_array.hpp>
#include <levk/core/thread_index.hpp>
#include <format>
#include <optional>

namespace levk {
namespace log {
/// \brief Log level.
enum class Level : int { eError, eWarn, eInfo, eDebug, eCOUNT_ };
inline constexpr auto level_to_char_v = EnumArray<Level, char>{'E', 'W', 'I', 'D'};

/// \brief Format text as a log message.
/// \param thread_index Thread index to use.
/// \param level Log level of the message.
/// \param tag Domain of the message.
/// \param text content of the message.
[[nodiscard]] auto format(thread::Index thread_index, Level level, std::string_view tag, std::string_view text) -> std::string;

/// \brief Print text to log outputs.
/// \param thread_index Thread index to use.
/// \param level Log level of the message.
/// \param tag Domain of the message.
/// \param text content of the message.
void print(thread::Index thread_index, Level level, std::string_view tag, std::string_view text);

/// \brief Set the maximum level for all log prints.
/// \param level Max level to set.
void set_max_level(Level level);

/// \brief Set or unset the maximum level for log prints for a tag domain.
/// \param tag The domain's tag.
/// \param level Maximum level to set, pass nullopt to unset.
void set_max_level_for(std::string_view tag, std::optional<Level> level);

/// \brief Get the maximum print level for a tag.
/// \param tag The domain's tag, or empty string.
/// \returns Domain tag's max level if configured, else max level for all log prints.
[[nodiscard]] auto get_max_level(std::string_view tag) -> Level;

/// \brief Reset all configured max levels.
void reset_max_levels();
} // namespace log

/// \brief Tagged logger.
class Logger {
  public:
	using Level = log::Level;

	explicit Logger(std::string tag = "Unnamed") : m_tag(std::move(tag)) {}

	template <typename... Args>
	void print(Level const level, std::format_string<Args...> fmt, Args&&... args) const {
		if (level > log::get_max_level(m_tag)) { return; }
		log::print(thread::get_index(), level, m_tag, std::format(fmt, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void error(std::format_string<Args...> fmt, Args&&... args) const {
		print(Level::eError, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void warn(std::format_string<Args...> fmt, Args&&... args) const {
		print(Level::eWarn, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void info(std::format_string<Args...> fmt, Args&&... args) const {
		print(Level::eInfo, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void debug(std::format_string<Args...> fmt, Args&&... args) const {
		print(Level::eDebug, fmt, std::forward<Args>(args)...);
	}

  private:
	std::string m_tag{"Unnamed"};
};
} // namespace levk
