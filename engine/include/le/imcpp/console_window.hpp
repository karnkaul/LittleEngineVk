#pragma once
#include <glm/vec4.hpp>
#include <le/core/ptr.hpp>
#include <le/core/string_trie.hpp>
#include <le/imcpp/str_buf.hpp>
#include <deque>
#include <optional>
#include <string>

struct ImGuiInputTextCallbackData;

namespace le::imcpp {
struct CommandLine { // NOLINT(cppcoreguidelines-virtual-class-destructor)
	struct Outcome {
		std::string response{};
		bool clear_log{};
	};

	virtual auto execute(std::string_view command) -> Outcome = 0;
	[[nodiscard]] virtual auto get_trie() const -> StringTrie const& = 0;
};

class ConsoleWindow {
  public:
	static constexpr std::size_t max_log_entries_v{100};
	static constexpr std::size_t max_history_v{100};

	void update(CommandLine& cli);
	void clear_log();

	struct {
		glm::vec4 command{1.0f, 1.0f, 0.0f, 1.0f};
		glm::vec4 response{0.5f, 0.5f, 0.5f, 1.0f}; // NOLINT
	} style{};

	std::string_view autocomplete_delim{" \t;"};
	std::size_t max_log_entries{max_log_entries_v};
	std::size_t max_history{max_history_v};
	bool show_window{true};

  private:
	static auto on_text_edit(ImGuiInputTextCallbackData* data) -> int;

	void update_input(CommandLine& cli);
	void update_log();

	void on_autocomplete(ImGuiInputTextCallbackData& data);
	void on_history(ImGuiInputTextCallbackData& data);

	struct Entry {
		std::string command{};
		std::string response{};
	};

	std::deque<Entry> m_log{};
	std::deque<std::string> m_history{};
	std::optional<std::size_t> m_history_index{};
	std::vector<std::string> m_candidates{};
	StrBuf m_input{};
	bool m_scroll_to_bottom{};

	Ptr<CommandLine> m_cli{};
};
} // namespace le::imcpp
