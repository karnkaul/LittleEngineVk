#pragma once
#include <core/io/path.hpp>
#include <core/span.hpp>
#include <ktl/fixed_vector.hpp>

namespace le::utils {
///
/// \brief Executes one or more cshell commands
///
class Shell {
  public:
#if defined(LEVK_OS_WINDOWS)
	inline static ktl::fixed_vector<int, 8> s_successCodes = {0, 1};
#else
	inline static ktl::fixed_vector<int, 8> s_successCodes = {0};
#endif
	inline static int s_redirectFailure = -100;

	///
	/// \brief Execute passed commands, redirect output if non-empty
	/// Execution will stop on the first failed command
	///
	Shell(Span<std::string const> commands, io::Path redirect = {});
	virtual ~Shell() = default;

	///
	/// \brief Check if a command processor is available
	///
	static bool supported();

	io::Path const& redirectPath() const noexcept { return m_redirect; }
	bool redirected() const noexcept { return supported() && !redirectPath().empty(); }
	///
	/// \brief Obtain redirected output, if file exists
	///
	std::string redirectOutput() const;

	///
	/// \brief Obtain overall result of execution
	///
	int result() const noexcept { return m_result; }
	bool success() const noexcept;
	explicit operator bool() const noexcept { return success(); }

  protected:
	io::Path m_redirect;
	int m_result;
};

///
/// \brief Executes one or more shell commands without any redirection
/// Note: Multiple concurrent instances may clobber each others' immediate output
///
class ShellInline : public Shell {
  public:
	ShellInline(Span<std::string const> commands) : Shell(commands, {}) {}
};

///
/// \brief Executes one or more shell commands, redirects output to a temporary file with a randomized suffix
/// Destructor removes redirect file if success
/// Note: Will block invoking thread without any output (it can be extracted before destruction if desired)
///
class ShellSilent : public Shell {
  public:
	ShellSilent(Span<std::string const> commands, std::string_view prefix = "_levk_shell");
	~ShellSilent() override;
};
} // namespace le::utils
