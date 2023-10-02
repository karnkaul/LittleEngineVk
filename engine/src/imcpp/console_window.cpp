#include <imgui.h>
#include <le/core/wrap.hpp>
#include <le/imcpp/common.hpp>
#include <le/imcpp/console_window.hpp>

namespace le::imcpp {
namespace {
constexpr auto trim(std::string_view in) -> std::string_view {
	while (!in.empty() && in.front() == ' ') { in = in.substr(1); }
	while (!in.empty() && in.back() == ' ') { in = in.substr(0, in.size() - 1); }
	return in;
}
} // namespace

void ConsoleWindow::update(console::Console& console) {
	if (!show_window) { return; }

	m_console = &console;
	ImGui::SetNextWindowSize({300.0f, 300.0f}, ImGuiCond_Once); // NOLINT
	if (auto w_main = Window{"console", &show_window}) {
		update_input();
		if (auto w_log = Window{w_main, "log", {}, {}, ImGuiWindowFlags_HorizontalScrollbar}) { update_log(); }
	}
	m_console = nullptr;
}

void ConsoleWindow::update_input() {
	assert(m_console);

	bool reclaim_focus = false;
	static constexpr ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
															ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory |
															ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackCharFilter;

	ImGui::Text("#"); // NOLINT
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##console_input", m_input.buf.data(), m_input.buf.size(), input_text_flags, &on_text_edit, this)) {
		m_console->submit(trim(m_input.view()));

		m_input.clear();
		m_cached.clear();
		m_history_index = {};
		m_scroll_to_top = true;
		reclaim_focus = true;
	}

	ImGui::SetItemDefaultFocus();
	if (reclaim_focus) { ImGui::SetKeyboardFocusHere(-1); }
}

void ConsoleWindow::update_log() {
	if (m_scroll_to_top) {
		m_scroll_to_top = false;
		ImGui::SetScrollHereY(1.0f);
	}
	for (auto const& entry : m_console->get_entries()) {
		if (!entry.request.empty()) {
			auto const c = cmd_rgba.to_vec4();
			ImGui::TextColored({c.x, c.y, c.z, c.w}, "%.*s", static_cast<int>(entry.request.size()), entry.request.data()); // NOLINT
		}
		if (!entry.response.error.empty()) {
			auto const c = err_rgba.to_vec4();
			ImGui::TextColored({c.x, c.y, c.z, c.w}, "%s", entry.response.error.c_str()); // NOLINT
		}
		if (!entry.response.message.empty()) {
			auto const c = msg_rgba.to_vec4();
			ImGui::TextColored({c.x, c.y, c.z, c.w}, "%s", entry.response.message.c_str()); // NOLINT
		}
		ImGui::Separator();
	}
}

auto ConsoleWindow::on_text_edit(ImGuiInputTextCallbackData* data) -> int {
	auto& self = *reinterpret_cast<ConsoleWindow*>(data->UserData); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	self.m_data = data;
	switch (data->EventFlag) {
	case ImGuiInputTextFlags_CallbackCharFilter: self.on_char_filter(); break;
	case ImGuiInputTextFlags_CallbackResize: self.m_input.extend(*data); break;
	case ImGuiInputTextFlags_CallbackCompletion: self.on_autocomplete(); break;
	case ImGuiInputTextFlags_CallbackHistory: self.on_history(); break;
	}
	self.m_data = nullptr;
	return 0;
}

void ConsoleWindow::on_char_filter() {
	m_cached.clear();
	m_history_index.reset();
}

void ConsoleWindow::on_autocomplete() {
	assert(m_console);
	auto ac = m_console->autocomplete(trim(m_input.view()), static_cast<std::size_t>(m_data->CursorPos));

	if (!ac.common_suffix.empty()) { m_data->InsertChars(m_data->CursorPos, ac.common_suffix.data(), ac.common_suffix.data() + ac.common_suffix.size()); }

	if (ac.candidates.size() == 1) {
		// exclusive match, insert space
		m_data->InsertChars(m_data->CursorPos, " ");
	} else if (!ac.candidates.empty()) {
		// multiple matches: log all of them
		m_console->add_entry(ac);
	}
}

void ConsoleWindow::on_history() {
	assert(m_console);
	auto const& history = m_console->get_history();
	if (history.empty()) { return; }

	auto const prev_history_index = m_history_index;
	if (!m_history_index) { m_cached = m_input.view(); }

	assert(m_data);
	if (m_data->EventKey == ImGuiKey_UpArrow) {
		if (!m_history_index) {
			m_history_index = 0;
		} else {
			m_history_index = increment_wrapped(*m_history_index, history.size() + 1);
		}
	} else if (m_data->EventKey == ImGuiKey_DownArrow) {
		if (!m_history_index) {
			m_history_index = history.size() - 1;
		} else {
			m_history_index = decrement_wrapped(*m_history_index, history.size() + 1);
		}
	}

	if (prev_history_index != m_history_index && m_history_index) {
		m_data->DeleteChars(0, m_data->BufTextLen);
		if (*m_history_index == history.size()) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			m_data->InsertChars(0, m_cached.data(), m_cached.data() + m_cached.size());
		} else {
			auto const& item = history.at(*m_history_index);
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			m_data->InsertChars(0, item.data(), item.data() + item.size());
		}
	}
}
} // namespace le::imcpp
