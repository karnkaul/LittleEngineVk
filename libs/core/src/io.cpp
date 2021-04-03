#include <filesystem>
#include <fstream>
#include <core/io.hpp>
#include <core/log.hpp>
#include <io_impl.hpp>
#include <kt/async_queue/async_queue.hpp>
#include <kt/kthread/kthread.hpp>

namespace le::io {
namespace {
struct FileLogger final {
	FileLogger();

	kt::kthread thread;
};

Path g_logFilePath;
kt::async_queue<std::string> g_queue;
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
	if (!oFile.good()) {
		return;
	}
	oFile.close();
	g_queue.active(true);
	logI("Logging to file: {}", absolute(g_logFilePath).generic_string());
	thread = kt::kthread([]() {
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
dl::config::on_log::token g_token;

void fileLog(std::string_view text, dl::level) {
	if (g_fileLogger) {
		g_queue.push(std::string(text));
	}
}
} // namespace

Service::Service(std::optional<Path> logFilePath) {
	if (logFilePath && !logFilePath->empty()) {
		g_logFilePath = std::move(*logFilePath);
		g_fileLogger = FileLogger();
		g_token = dl::config::g_on_log.add(&fileLog);
		m_bActive = true;
	}
}

Service::Service(Service&& rhs) : m_bActive(std::exchange(rhs.m_bActive, false)) {
}

Service& Service::operator=(Service&& rhs) {
	if (&rhs != this) {
		destroy();
		m_bActive = std::exchange(rhs.m_bActive, false);
	}
	return *this;
}

Service::~Service() {
	destroy();
}

void Service::destroy() {
	impl::deinitPhysfs();
	if (g_fileLogger && m_bActive) {
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
} // namespace le::io
