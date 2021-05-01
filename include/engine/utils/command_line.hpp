#pragma once
#include <array>
#include <unordered_map>
#include <core/io/cmd_interpreter.hpp>
#include <engine/utils/exec.hpp>
#include <kt/uint_flags/uint_flags.hpp>

namespace le::utils {
class CommandLine : private io::CmdInterpreter {
  public:
	using io::CmdInterpreter::Args;
	using io::CmdInterpreter::Cmd;
	using io::CmdInterpreter::Expr;
	using ExecMap = std::unordered_map<Cmd, Exec, Cmd::Hasher>;

	inline static std::string_view id_help = "help";
	inline static std::string_view id_interactive = "interactive";
	inline static std::string_view id_run = "run";
	inline static std::string_view id_quit = "quit";

	static std::vector<Expr> parse(View<std::string_view> tokens);

	CommandLine(ExecMap execs);

	bool map(Cmd cmd, Exec exec);
	bool execute(View<Expr> expressions, bool& boot);

  private:
	enum Flag { eInteractive = 1 << 0, eQuit = 1 << 1, eBreak = 1 << 2 };

	struct ExecEntry {
		Ref<Exec const> exec;
		Ref<Cmd const> cmd;
		char ch = '0';
	};
	struct Ignore {
		inline static constexpr std::array interacting = {&id_interactive};
		inline static constexpr std::array notInteracting = {&id_run, &id_quit};
	};
	static constexpr Ignore ignore{};

	std::vector<ExecEntry> execEntries() const;
	bool interact(View<ExecEntry> e);
	void help(View<ExecEntry> e) const;

	ExecMap m_map;
	kt::uint_flags<> m_flags{};
};
} // namespace le::utils
