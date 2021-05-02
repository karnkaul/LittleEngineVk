#pragma once
#include <functional>
#include <string>
#include <unordered_set>
#include <core/ref.hpp>
#include <core/span.hpp>

namespace le::io {
class CmdInterpreter {
  public:
	struct Cmd {
		std::string id;
		bool args = false;

		struct Hasher {
			std::size_t operator()(Cmd const& cmd) const;
		};
	};

	using Args = std::string;
	using Expr = std::pair<std::string, Args>;
	using CmdSet = std::unordered_set<Cmd, Cmd::Hasher>;

	struct Entry {
		Ref<CmdInterpreter::Cmd const> cmd;
		char ch = '0';
	};

	struct Out {
		std::string args;
		Ref<Cmd const> cmd;
	};

	static constexpr bool match(std::string_view str, std::string_view id, char ch) noexcept;
	static std::vector<Entry> entries(CmdInterpreter::CmdSet const& set);
	static Entry const* find(View<Entry> entries, std::string_view str);

	std::vector<Out> interpret(std::vector<Expr>& out_expressions) const;

	CmdSet m_set;
};

// impl

inline std::size_t CmdInterpreter::Cmd::Hasher::operator()(Cmd const& cmd) const {
	return std::hash<std::string>{}(cmd.id);
}

inline bool operator==(CmdInterpreter::Cmd const& a, CmdInterpreter::Cmd const& b) noexcept {
	return a.id == b.id;
}

constexpr bool CmdInterpreter::match(std::string_view str, std::string_view id, char ch) noexcept {
	return str == id || (str.size() == 1 && ch != '0' && ch == str[0]);
}
} // namespace le::io
