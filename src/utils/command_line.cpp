#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <core/ensure.hpp>
#include <core/utils/string.hpp>
#include <engine/utils/command_line.hpp>

namespace le::utils {
namespace {
std::string header(std::string_view id, bool args, char ch, bool prefix) {
	int w = 0;
	std::stringstream str;
	if (ch != '0') {
		if (prefix) {
			str << '-';
			++w;
		}
		str << ch << ", ";
		w += 3;
	}
	if (prefix) {
		str << "--";
		w += 2;
	}
	str << id;
	if (args) {
		str << "=...";
	}
	return str.str();
}
} // namespace

CommandLine::CommandLine(ExecMap args) : m_map(std::move(args)) {
	Exec help;
	help.label = "print this text";
	help.callback = [this](View<std::string_view>) {
		this->help(execEntries());
		if (m_flags.all(Flag::eInteractive)) {
			m_flags.update(0, Flag::eQuit);
		} else {
			m_flags.update(Flag::eQuit, 0);
		}
	};
	Exec interactive;
	interactive.label = "enter interactive mode";
	interactive.callback = [this](View<std::string_view>) { m_flags.update(Flag::eInteractive); };
	Exec run;
	run.label = "run application";
	run.callback = [this](View<std::string_view>) { m_flags.update(Flag::eBreak, Flag::eQuit); };
	Exec quit;
	quit.label = "quit";
	quit.callback = [this](View<std::string_view>) { m_flags.update(Flag::eQuit | Flag::eBreak); };
	m_map[{std::string(id_help)}] = std::move(help);
	m_map[{std::string(id_interactive)}] = std::move(interactive);
	m_map[{std::string(id_run)}] = std::move(run);
	m_map[{std::string(id_quit)}] = std::move(quit);
	for (auto& arg : m_map) {
		m_set.insert(arg.first);
	}
}

bool CommandLine::map(Cmd cmd, Exec exec) {
	if (!cmd.id.empty() && cmd.id != id_help && cmd.id != id_interactive && cmd.id != id_run && cmd.id != id_quit) {
		m_set.insert(cmd);
		m_map[std::move(cmd)] = std::move(exec);
		return true;
	}
	return false;
}

bool CommandLine::execute(View<Expr> expressions, bool& boot) {
	m_flags.update(0, Flag::eInteractive);
	auto const cmds = CmdInterpreter::interpret(expressions);
	for (Out const& cmd : cmds) {
		auto& arg = m_map[cmd.cmd];
		if (arg.callback) {
			arg.callback(utils::tokenise<16>(cmd.args, ','));
		}
	}
	auto const entries = execEntries();
	if (m_flags.all(Flag::eInteractive)) {
		help(entries);
		return interact(entries);
	}
	return !m_flags.all(Flag::eQuit) && boot;
}

std::vector<CommandLine::ExecEntry> CommandLine::execEntries() const {
	std::vector<CmdInterpreter::Entry> const ies = entries(m_set);
	std::vector<ExecEntry> ret;
	ret.reserve(ies.size());
	for (auto const& ie : ies) {
		if (auto it = m_map.find(ie.cmd); it != m_map.end()) {
			ret.push_back({it->second, ie.cmd, ie.ch});
		}
	}
	return ret;
}

bool CommandLine::interact(View<ExecEntry> es) {
	m_flags.update(0, Flag::eQuit | Flag::eBreak);
	std::cout << '\n';
	do {
		std::cout << ':';
		std::string str;
		std::getline(std::cin, str);
		std::size_t const idx = str.find('=');
		auto const id = idx < str.size() ? str.substr(0, idx) : str;
		if (idx < str.size()) {
			str = str.substr(idx + 1);
		}
		auto const params = utils::tokenise<16>(str, ' ');
		for (auto const& entry : es) {
			if (!id.empty() && match(id, entry.cmd.get().id, entry.ch)) {
				if (entry.exec.get().callback) {
					entry.exec.get().callback(params);
					std::cout << '\n';
				}
			}
		}
	} while (!m_flags.all(Flag::eBreak));
	return !m_flags.all(Flag::eQuit);
}

bool any(io::CmdInterpreter::Cmd const& cmd, View<std::string_view*> str) noexcept {
	return std::any_of(str.begin(), str.end(), [&cmd](std::string_view* s) { return *s == cmd.id; });
}

void CommandLine::help(View<ExecEntry> es) const {
	std::stringstream ret;
	std::vector<std::pair<std::string, std::string_view>> vec;
	bool const interactive = m_flags.all(Flag::eInteractive);
	for (auto const& entry : es) {
		if ((interactive && any(entry.cmd, ignore.interacting)) || (!interactive && any(entry.cmd, ignore.notInteracting))) {
			continue;
		}
		vec.push_back({header(entry.cmd.get().id, entry.cmd.get().args, entry.ch, !interactive), entry.exec.get().label});
	}
	std::size_t width = 0;
	for (auto& [h, _] : vec) {
		width = std::max(width, h.size() + 3);
	}
	for (auto& [h, l] : vec) {
		ret << h << std::setw(int(width - h.size())) << '\t' << l << '\n';
	}
	std::cout << '\n' << ret.str();
}
} // namespace le::utils
