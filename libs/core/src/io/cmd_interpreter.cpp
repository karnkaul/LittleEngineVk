#include <core/io/cmd_interpreter.hpp>
#include <core/utils/algo.hpp>
#include <core/utils/string.hpp>

namespace le::io {
CmdInterpreter::Entry const* CmdInterpreter::find(View<Entry> entries, std::string_view str) {
	ENSURE(!str.empty(), "Empty id");
	for (Entry const& entry : entries) {
		if (match(str, entry.cmd.get().id, entry.ch)) {
			return &entry;
		}
	}
	return nullptr;
}

std::vector<CmdInterpreter::Entry> CmdInterpreter::entries(CmdInterpreter::CmdSet const& set) {
	std::unordered_map<char, int> firsts;
	for (auto const& [id, _] : set) {
		ENSURE(!id.empty(), "Empty ID");
		++firsts[id[0]];
	}
	std::vector<Entry> ret;
	ret.reserve(set.size());
	for (auto const& cmd : set) {
		Entry entry{cmd};
		char const ch = cmd.id[0];
		if (!cmd.args && firsts[ch] == 1) {
			entry.ch = ch;
		}
		ret.push_back(entry);
	}
	return ret;
}

std::vector<CmdInterpreter::Out> CmdInterpreter::interpret(std::vector<Expr>& out_expressions) const {
	std::vector<CmdInterpreter::Out> ret;
	auto const es = entries(m_set);
	for (auto& expr : out_expressions) {
		if (Entry const* entry = find(es, expr.first)) {
			ret.push_back({expr.second, entry->cmd});
			expr = {};
		}
	}
	utils::erase_if(out_expressions, [](Expr const& e) { return e.first.empty(); });
	return ret;
}
} // namespace le::io
