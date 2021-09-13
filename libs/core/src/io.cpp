#include <filesystem>
#include <fstream>
#include <core/io.hpp>
#include <core/log.hpp>
#include <ktl/async_queue.hpp>
#include <ktl/kthread.hpp>

namespace le::io {
namespace {
struct FileLogger final {
	FileLogger();

	ktl::kthread thread;
};

Path g_logFilePath;
ktl::async_queue<std::string> g_queue;
void dumpToFile(Path const& path, std::string const& str);
FileLogger::FileLogger() {
	std::ifstream iFile(g_logFilePath.generic_string());
	if (iFile.good()) {
		iFile.close();
		Path backup(g_logFilePath.generic_string());
		backup += ".bak";
		std::rename(g_logFilePath.generic_string().data(), backup.generic_string().data());
	}
	std::ofstream oFile(g_logFilePath.generic_string());
	if (!oFile.good()) { return; }
	oFile.close();
	g_queue.active(true);
	logI("Logging to file: {}", absolute(g_logFilePath).generic_string());
	thread = ktl::kthread([]() {
		while (auto str = g_queue.pop()) {
			*str += "\n";
			dumpToFile(g_logFilePath, *str);
		}
	});
	return;
}

void dumpToFile(Path const& path, std::string const& str) {
	if (!path.empty() && !str.empty()) {
		std::ofstream file(path.generic_string(), std::ios_base::app);
		file.write(str.data(), (std::streamsize)str.length());
	}
	return;
}

std::optional<FileLogger> g_fileLogger;
dl::config::on_log::handle g_token;

[[maybe_unused]] void fileLog(std::string_view text, dl::level) {
	if (g_fileLogger) { g_queue.push(std::string(text)); }
}
} // namespace

Service::Service([[maybe_unused]] Path logFilePath) {
	if (!logFilePath.empty()) {
		g_token = dl::config::g_on_log.add(&fileLog);
		g_logFilePath = std::move(logFilePath);
		g_fileLogger = FileLogger();
		m_active = true;
	}
}

Service::~Service() {
	if (g_fileLogger && m_active) {
		logI("File Logging terminated");
		g_token = {};
		g_queue.active(false);
		g_fileLogger.reset();
		auto residue = g_queue.clear();
		std::string residueStr;
		for (auto& str : residue) {
			residueStr = std::move(str);
			residueStr += "\n";
		}
		dumpToFile(g_logFilePath, residueStr);
	}
}

void Service::exchg(Service& lhs, Service& rhs) noexcept { std::swap(lhs.m_active, rhs.m_active); }
} // namespace le::io
