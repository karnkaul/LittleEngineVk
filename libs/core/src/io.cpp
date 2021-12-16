#include <core/io.hpp>
#include <core/log.hpp>
#include <dumb_log/file_logger.hpp>
#include <ktl/async_queue.hpp>
#include <ktl/kthread.hpp>
#include <filesystem>
#include <fstream>

namespace le::io {
Service::Service(Path logFilePath, LogChannel active) {
	if (!logFilePath.empty()) {
		dlog::set_channels({active});
		std::string const path = logFilePath.generic_string();
		logI("[io] Logging to file: [{}]", path);
		m_handle = dlog::file_logger::attach(path.data());
	}
}
} // namespace le::io
