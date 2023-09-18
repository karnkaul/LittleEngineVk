#include <imgui.h>
#include <le/core/wrap.hpp>
#include <le/imcpp/common.hpp>
#include <le/imcpp/console_window.hpp>

namespace le::imcpp {
void ConsoleWindow::update(CommandLine& cli) {
	if (!show_window) { return; }

	ImGui::SetNextWindowSize({300.0f, 300.0f}, ImGuiCond_Once); // NOLINT
	if (auto w_main = Window{"console", &show_window}) {
		update_input(cli);
		if (auto w_log = Window{w_main, "log", {}, {}, ImGuiWindowFlags_HorizontalScrollbar}) { update_log(); }
	}
}

void ConsoleWindow::clear_log() { m_log.clear(); }

void ConsoleWindow::update_input(CommandLine& cli) {
	bool reclaim_focus = false;
	static constexpr ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
															ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory |
															ImGuiInputTextFlags_CallbackResize;

	ImGui::Text("#"); // NOLINT
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.0f);
	m_cli = &cli;
	if (ImGui::InputText("##console_input", m_input.buf.data(), m_input.buf.size(), input_text_flags, &on_text_edit, this)) {
		auto const text = m_input.view();
		auto outcome = cli.execute(text);
		m_history.emplace_back(text);
		while (m_history.size() > max_history) { m_history.pop_front(); }
		m_log.push_front(Entry{std::string{text}, std::move(outcome.response)});
		if (outcome.clear_log) { clear_log(); }
		while (m_log.size() > max_log_entries) { m_log.pop_back(); }
		m_input.clear();
		m_history_index = {};
		m_scroll_to_bottom = true;
		reclaim_focus = true;
	}
	m_cli = {};

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus) {
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
	}
}

void ConsoleWindow::update_log() {
	for (auto const& [command, response] : m_log) {
		if (!command.empty()) {
			auto const& c = style.command;
			ImGui::TextColored({c.x, c.y, c.z, c.w}, "# %s", command.c_str()); // NOLINT
		}
		auto const& c = style.response;
		ImGui::TextColored({c.x, c.y, c.z, c.w}, "%s", response.c_str()); // NOLINT
		ImGui::Separator();
	}
	if (m_scroll_to_bottom) {
		m_scroll_to_bottom = false;
		ImGui::SetScrollHereY(1.0f);
	}
}

auto ConsoleWindow::on_text_edit(ImGuiInputTextCallbackData* data) -> int {
	auto& self = *reinterpret_cast<ConsoleWindow*>(data->UserData); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	switch (data->EventFlag) {
	case ImGuiInputTextFlags_CallbackResize: self.m_input.extend(*data); break;
	case ImGuiInputTextFlags_CallbackCompletion: self.on_autocomplete(*data); break;
	case ImGuiInputTextFlags_CallbackHistory: self.on_history(*data); break;
	}
	return 0;
}

void ConsoleWindow::on_autocomplete(ImGuiInputTextCallbackData& data) {
	auto const cursor_pos = static_cast<std::size_t>(data.CursorPos);
	auto word = std::string_view{data.Buf, cursor_pos};
	auto word_start = word.find_last_of(autocomplete_delim);
	if (word_start != std::string_view::npos) {
		word = word.substr(++word_start);
	} else {
		word_start = 0;
	}

	m_candidates.clear();
	m_cli->autocomplete(word, m_candidates);

	if (!m_candidates.empty()) {
		if (m_candidates.size() == 1) {
			auto const to_insert = m_candidates.front();
			if (word_start + to_insert.size() < m_input.capacity()) {
				data.DeleteChars(static_cast<int>(word_start), static_cast<int>(word.size()));
				data.InsertChars(static_cast<int>(word_start), to_insert.data(), to_insert.data() + to_insert.size());
				data.InsertChars(data.CursorPos, " ");
			}
		} else {
			auto response = std::string{};
			for (auto const candidate : m_candidates) { std::format_to(std::back_inserter(response), "{}\n", candidate); }
			m_log.push_front(Entry{.response = std::move(response)});
		}
	}
}

void ConsoleWindow::on_history(ImGuiInputTextCallbackData& data) {
	if (m_history.empty()) { return; }

	auto const prev_history_index = m_history_index;
	if (data.EventKey == ImGuiKey_UpArrow) {
		if (!m_history_index) {
			m_history_index = m_history.size() - 1;
		} else {
			m_history_index = decrement_wrapped(*m_history_index, m_history.size());
		}
	} else if (data.EventKey == ImGuiKey_DownArrow) {

		if (!m_history_index) {
			m_history_index = 0;
		} else {
			m_history_index = increment_wrapped(*m_history_index, m_history.size());
		}
	}

	if (prev_history_index != m_history_index && m_history_index) {
		auto const& item = m_history.at(*m_history_index);
		data.DeleteChars(0, data.BufTextLen);
		data.InsertChars(0, item.c_str());
	}
}
} // namespace le::imcpp
