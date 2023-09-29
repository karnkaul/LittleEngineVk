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

auto ConsoleWindow::get_command_line() const -> std::string_view { return trim(m_input.view()); }

auto ConsoleWindow::get_cursor() const -> std::size_t {
	if (m_data == nullptr) { return 0; }

	return static_cast<std::size_t>(m_data->CursorPos);
}

auto ConsoleWindow::insert(std::string_view const text) -> bool {
	if (m_data == nullptr) { return false; }

	if (m_input.view().size() + text.size() >= m_input.capacity()) { return false; }

	m_data->InsertChars(m_data->CursorPos, text.data(), text.data() + text.size());
	return true;
}

void ConsoleWindow::update(cli::Responder& responder) {
	if (!show_window) { return; }

	m_responder = &responder;
	ImGui::SetNextWindowSize({300.0f, 300.0f}, ImGuiCond_Once); // NOLINT
	if (auto w_main = Window{"console", &show_window}) {
		update_input();
		if (auto w_log = Window{w_main, "log", {}, {}, ImGuiWindowFlags_HorizontalScrollbar}) { update_log(); }
	}
	m_responder = nullptr;
}

void ConsoleWindow::update_input() {
	assert(m_responder);

	bool reclaim_focus = false;
	static constexpr ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
															ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory |
															ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackCharFilter;

	ImGui::Text("#"); // NOLINT
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##console_input", m_input.buf.data(), m_input.buf.size(), input_text_flags, &on_text_edit, this)) {
		m_responder->submit(*this);

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
	for (auto const& entry : m_entries) {
		if (!entry.cmd.empty()) {
			auto const c = cmd_rgba.to_vec4();
			ImGui::TextColored({c.x, c.y, c.z, c.w}, "%.*s", static_cast<int>(entry.cmd.size()), entry.cmd.data()); // NOLINT
		}
		auto const c = entry.rgba.to_vec4();
		ImGui::TextColored({c.x, c.y, c.z, c.w}, "%s", entry.response.c_str()); // NOLINT
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
	assert(m_responder);
	m_responder->autocomplete(*this);
}

void ConsoleWindow::on_history() {
	if (m_history.empty()) { return; }

	auto const prev_history_index = m_history_index;
	if (!m_history_index) { m_cached = m_input.view(); }

	assert(m_data);
	if (m_data->EventKey == ImGuiKey_UpArrow) {
		if (!m_history_index) {
			m_history_index = 0;
		} else {
			m_history_index = increment_wrapped(*m_history_index, m_history.size() + 1);
		}
	} else if (m_data->EventKey == ImGuiKey_DownArrow) {
		if (!m_history_index) {
			m_history_index = m_history.size() - 1;
		} else {
			m_history_index = decrement_wrapped(*m_history_index, m_history.size() + 1);
		}
	}

	if (prev_history_index != m_history_index && m_history_index) {
		m_data->DeleteChars(0, m_data->BufTextLen);
		if (*m_history_index == m_history.size()) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			m_data->InsertChars(0, m_cached.data(), m_cached.data() + m_cached.size());
		} else {
			auto const& item = m_history.at(*m_history_index);
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			m_data->InsertChars(0, item.data(), item.data() + item.size());
		}
	}
}
} // namespace le::imcpp
