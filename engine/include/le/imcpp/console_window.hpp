#pragma once
#include <glm/vec4.hpp>
#include <le/console/console.hpp>
#include <le/core/ptr.hpp>
#include <le/graphics/rgba.hpp>
#include <le/imcpp/str_buf.hpp>
#include <deque>
#include <optional>

struct ImGuiInputTextCallbackData;

namespace le::imcpp {
class ConsoleWindow {
  public:
	static constexpr auto cmd_rgba_v = graphics::Rgba{.channels = {0xff, 0xff, 0x0, 0xff}};
	static constexpr auto msg_rgba_v = graphics::Rgba{.channels = {0xff, 0xff, 0xff, 0xff}};
	static constexpr auto err_rgba_v = graphics::Rgba{.channels = {0xff, 0x77, 0x0, 0xff}};

	void update(console::Console& console);

	graphics::Rgba cmd_rgba{cmd_rgba_v};
	graphics::Rgba msg_rgba{msg_rgba_v};
	graphics::Rgba err_rgba{err_rgba_v};
	bool show_window{true};

  private:
	static auto on_text_edit(ImGuiInputTextCallbackData* data) -> int;

	void update_input();
	void update_log();

	void on_char_filter();
	void on_autocomplete();
	void on_history();

	std::optional<std::size_t> m_history_index{};
	StrBuf m_input{};
	std::string m_cached{};
	bool m_scroll_to_top{};

	Ptr<ImGuiInputTextCallbackData> m_data{};
	Ptr<console::Console> m_console{};
};
} // namespace le::imcpp
