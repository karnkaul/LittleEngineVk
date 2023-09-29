#include <le/cli/console.hpp>
#include <format>

namespace le::cli {
void Console::add_entry(std::string_view const cmd, std::string response, graphics::Rgba rgba) {
	auto command = std::string{};
	if (!cmd.empty()) {
		m_history.emplace_front(cmd);
		command = std::format("# {}", cmd);
	}

	m_entries.push_front(Entry{.cmd = std::move(command), .response = std::move(response), .rgba = rgba});
	while (m_entries.size() > max_entries) { m_entries.pop_back(); }
}

void Console::clear_log() { m_entries.clear(); }
} // namespace le::cli
