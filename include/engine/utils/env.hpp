#pragma once
#include <core/io/cmd_interpreter.hpp>
#include <core/io/path.hpp>
#include <engine/utils/exec.hpp>
#include <kt/result/result.hpp>

namespace le::utils {
class Env {
  public:
	using Cmd = io::CmdInterpreter::Cmd;
	using Expr = io::CmdInterpreter::Expr;
	using ExecMap = std::unordered_map<Cmd, utils::Exec, Cmd::Hasher>;

	static bool init(int argc, char* const argv[], ExecMap execs);
	///
	/// \brief Obtain full path to directory containing pattern, traced from the executable path
	/// \param pattern sub-path to match against
	/// \param start Dir to start search from
	/// \param maxHeight maximum recursive depth
	///
	static kt::result<io::Path, std::string> findData(io::Path pattern = "data", u8 maxHeight = 10);
	static View<Expr> unusedExpressions() noexcept;
	static void runUnusedArgs(ExecMap execs);

  private:
	inline static std::vector<Expr> s_unused;
};

// impl

inline View<Env::Expr> Env::unusedExpressions() noexcept {
	return s_unused;
}
} // namespace le::utils
