#include <ktl/async/kthread.hpp>
#include <levk/core/io/path.hpp>
#include <levk/core/log.hpp>
#include <levk/core/maths.hpp>
#include <levk/core/utils/shell.hpp>
#include <levk/core/utils/string.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace le::utils {
namespace {
std::string randomPath(std::string_view prefix, std::string_view ext) { return fmt::format("{}_{}.{}", prefix, maths::randomRange(1000, 9999), ext); }

std::unordered_map<std::string_view, ktl::fixed_vector<int, 8>> const g_successCodeMap = {
	{"explorer", {0, 1}},
};

std::string_view extractCmd(std::string_view command) noexcept {
	for (std::size_t i = 0; i < command.size(); ++i) {
		if (std::isspace(static_cast<unsigned char>(command[i]))) { return command.substr(0, i); }
	}
	return command;
}

bool isSuccess(std::string_view cmd, int retCode) noexcept {
	if (auto it = g_successCodeMap.find(cmd); it != g_successCodeMap.end()) {
		return std::find(it->second.begin(), it->second.end(), retCode) != it->second.end();
	}
	return retCode == 0;
}
} // namespace

bool Shell::supported() { return std::system(nullptr) != 0; }

Shell::Shell(char const* command, char const* outputFile, std::optional<int> successCode) { execute(command, outputFile, successCode); }

bool Shell::execute(char const* command, char const* outputFile, std::optional<int> successCode) {
	m_redirected = outputFile && *outputFile;
	if (supported()) {
		if (!m_redirected) {
			logD("[Shell] Executing: {}", command);
			std::cout.flush();
			std::cerr.flush();
			m_result = std::system(command);
		} else {
			if (auto f = std::ofstream(outputFile, std::ios::app)) {
				auto const cmd = fmt::format("{} >> {} 2>&1", command, outputFile);
				logD("[Shell] Executing: {}", cmd);
				f << "\n== [levk shell] executing: " << command << "\n\n";
				f.close();
				m_result = std::system(cmd.data());
			} else {
				m_result = eRedirectFailure;
			}
			if (auto f = std::ifstream(outputFile)) { m_output = toString(f); }
		}
		if (m_result < 0) {
			m_success = false;
		} else {
			m_success = successCode.has_value() ? m_result == *successCode : isSuccess(extractCmd(command), m_result);
		}
	} else {
		m_result = eUnsupportedShell;
		m_success = false;
	}
	return m_success;
}

ShellSilent::ShellSilent(char const* command, std::string_view prefix, std::optional<int> successCode) { execute(command, prefix, successCode); }

bool ShellSilent::execute(char const* command, std::string_view prefix, std::optional<int> successCode) {
	auto const outputFile = randomPath(prefix, "txt");
	auto const file = io::absolute(outputFile);
	if (Shell::execute(command, outputFile.data(), successCode)) {
		if (io::is_regular_file(file)) {
			static constexpr int maxTries = 1000 / 50;
			for (int i = 0; i < maxTries; ++i) {
				if (io::remove(file)) {
					logD("[Shell] Deleted [{}]", outputFile);
					break;
				}
				ktl::kthread::sleep_for(std::chrono::milliseconds(50));
			}
			if (io::is_regular_file(file)) { logW("[Shell] Failed to delete [{}]!", file.generic_string()); }
		}
		return true;
	} else {
		logW("[Shell] command failure; return code: [{}], redirect file: [{}]", result(), file.generic_string());
		return false;
	}
}
} // namespace le::utils
