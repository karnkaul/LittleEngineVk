#include <le/console/console.hpp>
#include <algorithm>
#include <array>

namespace le::console {
namespace {
struct Request {
	std::string_view property{};
	std::string_view value{};

	static auto make(std::string_view const text) -> Request {
		auto const space = text.find(' ');
		if (space == std::string_view::npos) { return {.property = text}; }
		return {.property = text.substr(0, space), .value = text.substr(space + 1)};
	}
};

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

void process_to(Entry& out, std::unordered_map<std::string_view, std::unique_ptr<Command>> const& map, std::string_view cmd, std::string_view value) {
	auto const it = map.find(cmd);
	if (it == map.end()) {
		out.response.error = "unrecognized command\nenter 'help' or press Tab";
		return;
	}

	out.response = it->second->process(value);
}
} // namespace

Console::Console()
	: m_builtins({
		  Builtin{"help", [](Console& console) { console.add_entry(console.autocomplete("", 0)); }},
		  Builtin{"clear", [](Console& console) { console.clear_log(); }},
	  }) {}

void Console::submit(std::string_view text) {
	if (text.empty()) { return; }
	auto const request = Request::make(text);
	auto entry = Entry{.request = std::string{text}};
	add_to_history(entry.request);
	if (!builtin(text)) {
		process_to(entry, m_commands, request.property, request.value);
		add_entry(std::move(entry));
	}
}

auto Console::autocomplete(std::string_view text, std::size_t cursor) const -> Autocomplete {
	auto const request = Request::make(text);
	if (cursor > request.property.size()) { return {}; }

	auto ret = Autocomplete{};
	auto const fragment = request.property.substr(0, cursor);
	auto commands = autocomplete(fragment);
	std::ranges::sort(commands);

	ret.candidates = get_builtin_names(fragment);
	ret.candidates.reserve(ret.candidates.size() + commands.size());
	std::ranges::move(commands, std::back_inserter(ret.candidates));

	ret.common_suffix = build_common_suffix(ret.candidates, fragment);

	return ret;
}

void Console::add_entry(Entry entry) {
	m_entries.push_front(std::move(entry));
	while (m_entries.size() > max_entries) { m_entries.pop_back(); }
}

void Console::add_entry(Autocomplete const& ac) {
	auto text = std::string{};
	for (auto const candidate : ac.candidates) { std::format_to(std::back_inserter(text), "- {}\n", candidate); }
	add_entry(Entry{.response = {.message = std::move(text)}});
}

void Console::add_to_history(std::string text) {
	m_history.push_front(std::move(text));
	while (m_history.size() > max_history) { m_history.pop_back(); }
}

auto Console::autocomplete(std::string_view fragment) const -> std::vector<std::string_view> {
	auto set = std::unordered_set<std::string_view>{};
	for (auto const& [name, _] : m_commands) {
		if (name.starts_with(fragment)) { set.insert(name); }
	}
	return {set.begin(), set.end()};
}

auto Console::builtin(std::string_view const cmd) -> bool {
	auto const func = [cmd](Builtin const& builtin) { return builtin.name == cmd; };
	if (auto const it = std::ranges::find_if(m_builtins, func); it != m_builtins.end()) {
		it->command(*this);
		return true;
	}
	return false;
}

auto Console::get_builtin_names(std::string_view const fragment) const -> std::vector<std::string_view> {
	auto ret = std::vector<std::string_view>{};
	for (auto const& [name, _] : m_builtins) {
		if (name.starts_with(fragment)) { ret.push_back(name); }
	}
	return ret;
}
} // namespace le::console
