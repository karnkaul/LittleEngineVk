#pragma once
#include <le/core/polymorphic.hpp>
#include <le/graphics/rgba.hpp>
#include <deque>
#include <span>
#include <string>
#include <utility>
#include <variant>

namespace le::cli {
class Console : public Polymorphic {
  public:
	static constexpr std::size_t max_entries_v{100};
	static constexpr std::size_t max_log_entries_v{100};
	static constexpr std::size_t max_history_v{100};
	static constexpr auto cmd_rgba_v = graphics::Rgba{.channels = {0xff, 0xff, 0x0, 0xff}};

	[[nodiscard]] virtual auto get_command_line() const -> std::string_view = 0;
	[[nodiscard]] virtual auto get_cursor() const -> std::size_t = 0;

	virtual auto insert(std::string_view text) -> bool = 0;

	void add_entry(std::string_view cmd, std::string response, graphics::Rgba rgba = graphics::white_v);
	void add_response(std::string response, graphics::Rgba rgba = graphics::white_v) { add_entry({}, std::move(response), rgba); }

	void clear_log();

	std::size_t max_entries{max_log_entries_v};
	std::size_t max_log_entries{max_log_entries_v};
	std::size_t max_history{max_history_v};
	graphics::Rgba cmd_rgba{cmd_rgba_v};

  protected:
	struct Entry {
		std::string cmd{};
		std::string response{};
		graphics::Rgba rgba{graphics::white_v};
	};

	std::deque<Entry> m_entries{};
	std::deque<std::string> m_history{};
};
} // namespace le::cli
