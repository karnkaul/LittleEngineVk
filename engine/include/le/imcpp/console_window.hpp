#pragma once
#include <glm/vec4.hpp>
#include <le/cli/responder.hpp>
#include <le/core/ptr.hpp>
#include <le/imcpp/str_buf.hpp>
#include <deque>
#include <optional>

struct ImGuiInputTextCallbackData;

namespace le::imcpp {
class ConsoleWindow : public cli::Console {
  public:
	[[nodiscard]] auto get_command_line() const -> std::string_view final;
	[[nodiscard]] auto get_cursor() const -> std::size_t final;

	auto insert(std::string_view text) -> bool final;

	void update(cli::Responder& responder);

	std::size_t max_log_entries{max_log_entries_v};
	std::size_t max_history{max_history_v};
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
	Ptr<cli::Responder> m_responder{};
};
} // namespace le::imcpp
