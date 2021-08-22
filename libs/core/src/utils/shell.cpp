#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <core/utils/shell.hpp>
#include <core/utils/string.hpp>

namespace le::utils {
namespace {
io::Path randomPath(std::string_view prefix, std::string_view ext) { return fmt::format("{}_{}.{}", prefix, maths::randomRange(1000, 9999), ext); }
} // namespace

bool Shell::supported() { return std::system(nullptr) != 0; }

Shell::Shell(Span<std::string const> commands, io::Path redirect) : m_redirect(std::move(redirect)), m_result(supported() ? 0 : -1) {
	if (supported()) {
		auto run = [](std::string const& path, std::string_view cmd) {
			if (path.empty()) {
				logD("[Shell] Executing: {}", cmd);
				std::cout.flush();
				std::cerr.flush();
				return std::system(cmd.data());
			}
			if (auto f = std::ofstream(path, std::ios::app)) {
				auto const cmd_ = fmt::format("{} >> {} 2>&1", cmd, path);
				logD("[Shell] Executing: {}", cmd_, path);
				f << "\n== [levk shell] executing: " << cmd << "\n\n";
				f.close();
				return std::system(cmd_.data());
			}
			return s_redirectFailure;
		};
		std::string const path = m_redirect.string();
		for (auto const& cmd : commands) {
			if (m_result = run(path, cmd); m_result != 0) { break; }
		}
	}
}

std::string Shell::redirectOutput() const {
	if (redirected()) {
		if (auto f = std::ifstream(m_redirect.string())) { return toString(f); }
	}
	return {};
}

bool Shell::success() const noexcept {
	for (int const s : s_successCodes) {
		if (m_result == s) { return true; }
	}
	return false;
}

ShellSilent::ShellSilent(Span<std::string const> commands, std::string_view prefix) : Shell(commands, randomPath(prefix, "txt")) {}

ShellSilent::~ShellSilent() {
	if (success()) {
		logD("[Shell] Deleting [{}]", m_redirect.generic_string());
		io::remove(m_redirect);
	}
}
} // namespace le::utils
