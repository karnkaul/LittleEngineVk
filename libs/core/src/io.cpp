#include <fstream>
#include <core/io.hpp>
#include <core/log.hpp>
#include <core/threads.hpp>
#include <io_impl.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le::io {
namespace {
struct FileLogger final {
	FileLogger();

	threads::TScoped thread;
};

std::filesystem::path g_logFilePath;
kt::async_queue<std::string> g_queue;
void dumpToFile(std::filesystem::path const& path, std::string const& str);

FileLogger::FileLogger() {
	std::ifstream iFile(g_logFilePath);
	if (iFile.good()) {
		iFile.close();
		std::filesystem::path backup(g_logFilePath);
		backup += ".bak";
		std::filesystem::rename(g_logFilePath, backup);
	}
	std::ofstream oFile(g_logFilePath);
	if (!oFile.good()) {
		return;
	}
	oFile.close();
	g_queue.active(true);
	logI("Logging to file: {}", std::filesystem::absolute(g_logFilePath).generic_string());
	thread = threads::newThread([]() {
		while (auto str = g_queue.pop()) {
			*str += "\n";
			dumpToFile(g_logFilePath, *str);
		}
	});
	return;
}

void dumpToFile(std::filesystem::path const& path, std::string const& str) {
	if (!path.empty() && !str.empty()) {
		std::ofstream file(path, std::ios_base::app);
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

Service::Service(std::optional<std::filesystem::path> logFilePath) {
	if (logFilePath && !logFilePath->empty()) {
		g_logFilePath = std::move(*logFilePath);
		g_fileLogger = FileLogger();
		g_token = dl::config::g_on_log.add(&fileLog);
	}
}

Service::~Service() {
	impl::deinitPhysfs();
	if (g_fileLogger) {
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
