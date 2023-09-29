#include <le/cli/driver.hpp>
#include <vector>

namespace le::cli {
namespace {
constexpr auto error_rgba_v = graphics::Rgba{.channels = {0xff, 0x77, 0x0, 0xff}};

[[nodiscard]] auto build_common_suffix(std::span<std::string_view const> candidates, std::string_view const fragment) -> std::string {
	if (candidates.empty()) { return {}; }
	if (candidates.size() == 1) { return std::string{candidates.front().substr(fragment.size())}; }

	auto const min_length = [&] {
		auto ret = std::string_view::npos;
		for (auto const& str : candidates) { ret = std::min(ret, str.size()); }
		return ret;
	}();
	auto const cmp = std::string_view{candidates.front()};
	auto const is_common_index = [candidates, cmp](std::size_t const index) {
		return std::ranges::all_of(candidates, [cmp = cmp, index](auto const& str) { return str.at(index) == cmp.at(index); });
	};
	auto const common_index = [&] {
		auto ret = std::string_view::npos;
		for (std::size_t index = 0; index < min_length; ++index) {
			if (!is_common_index(index)) { return ret; }
			ret = index;
		}
		return ret;
	}();
	if (common_index == std::string_view::npos) { return {}; }

	auto ret = candidates.front().substr(0, common_index + 1);
	return std::string{ret.substr(fragment.size())};
}

template <typename M>
[[nodiscard]] auto build_candidates(M const& map, std::string_view const fragment) -> std::vector<std::string_view> {
	auto ret = std::vector<std::string_view>{};

	if (fragment.empty()) {
		for (auto const& [name, _] : map) { ret.push_back(name); }
		return ret;
	}

	for (auto const& [name, _] : map) {
		if (name.starts_with(fragment)) { ret.push_back(name); }
	}

	return ret;
}
} // namespace

void Driver::autocomplete(Console& console) const {
	auto const line = console.get_command_line();
	auto cmd_fragment = std::string_view{};
	if (!line.empty()) {
		auto const first_word = Args{line}.next();
		auto const cursor = console.get_cursor();
		if (cursor > first_word.size()) {
			// currently we only autocomplete the first word
			return;
		}
		cmd_fragment = first_word.substr(0, cursor);
	}

	auto const candidates = build_candidates(m_commands, cmd_fragment);
	auto const common_suffix = build_common_suffix(candidates, cmd_fragment);

	if (!common_suffix.empty()) { console.insert(common_suffix); }

	if (candidates.size() == 1) {
		// exclusive match: insert a space at the end
		console.insert(" ");
	} else if (!candidates.empty()) {
		// multiple matches: log all of them
		auto text = std::string{};
		for (auto const candidate : candidates) { std::format_to(std::back_inserter(text), "{}\n", candidate); }
		console.add_response(std::move(text));
	}
}

void Driver::submit(Console& console) {
	auto const line = console.get_command_line();
	if (line.empty()) { return; }

	auto args = Args{line};
	assert(!args.is_empty());

	auto const cmd = args.next();
	auto const it = m_commands.find(cmd);
	if (it == m_commands.end()) {
		console.add_entry(line, "unrecognized command", error_rgba_v);
		return;
	}

	auto& command = *it->second;
	command.execute(args);
	if ((command.get_flags() & Command::eClearLog) == Command::eClearLog) {
		console.clear_log();
	} else {
		console.add_entry(line, command.get_response().text, command.get_response().rgba);
	}
}

void Driver::add_command(std::unique_ptr<Command> command) {
	if (!command) { return; }
	auto const name = command->get_name();
	m_commands.insert_or_assign(name, std::move(command));
}
} // namespace le::cli
