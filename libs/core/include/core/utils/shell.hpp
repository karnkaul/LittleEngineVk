#pragma once
#include <optional>
#include <string>

namespace le::utils {
///
/// \brief Executes a shell command
///
/// Supports output redirection (and subsequent extraction) and custom success codes (must be positive)
///
class Shell {
  public:
	///
	/// \brief Value stored as result on meta-execution errors
	///
	enum {
		eSuccess = 0,
		eUnsupportedShell = -10,
		eRedirectFailure = -100,
	};

	Shell() = default;
	///
	/// \brief Execute passed command, redirect output if non-empty
	///
	explicit Shell(char const* command, char const* outputFile = {}, std::optional<int> successCode = std::nullopt);
	virtual ~Shell() = default;

	///
	/// \brief Check if a command processor is available
	///
	static bool supported();

	///
	/// \brief Execute command and obtain whether its return code matched a success code
	///
	bool execute(char const* command, char const* outputFile = {}, std::optional<int> successCode = std::nullopt);
	///
	/// \brief Check if output has been redirected to a file
	///
	bool redirected() const noexcept { return supported() && m_redirected; }
	///
	/// \brief Obtain redirected output
	///
	std::string_view output() const { return m_output; }

	///
	/// \brief Obtain overall result of execution
	///
	int result() const noexcept { return m_result; }
	bool success() const noexcept { return m_success; }
	explicit operator bool() const noexcept { return success(); }

  protected:
	std::string m_output;
	int m_result{};
	bool m_success{};
	bool m_redirected{};
};

///
/// \brief Executes a shell command without any redirection; useful for single-threaded calls
///
/// Note: Multiple concurrent instances may clobber each others' immediate output
///
class ShellInline : public Shell {
  public:
	ShellInline() = default;

	explicit ShellInline(char const* command) : Shell(command, {}) {}
};

///
/// \brief Executes a shell command, redirects output to a temporary file with a randomized suffix; useful for parallelized calls
///
/// Note: Will block invoking thread without any logs/output (can be extracted before destruction if desired)
/// Output file is only deleted if execution was successful
///
class ShellSilent : public Shell {
  public:
	ShellSilent() = default;
	explicit ShellSilent(char const* command, std::string_view prefix = "_levk_shell", std::optional<int> successCode = std::nullopt);

	bool execute(char const* command, std::string_view prefix = "_levk_shell", std::optional<int> successCode = std::nullopt);
};
} // namespace le::utils
