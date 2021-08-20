#include <filesystem>
#include <fstream>
#include <fmt/format.h>
#include <build_version.hpp>
#include <core/services.hpp>
#include <dumb_json/json.hpp>
#include <engine/engine.hpp>
#include <engine/utils/error_handler.hpp>

namespace le::utils {
namespace stdfs = std::filesystem;

namespace {
template <typename T>
void add(dj::json& root, std::string const& key, T value) {
	dj::json property;
	property.set(std::move(value));
	root.insert(key, std::move(property));
}
} // namespace

Version const ErrInfo::build = g_buildVersion;

ErrInfo::ErrInfo(std::string message, SrcInfo const& source) : source(source), message(std::move(message)), timestamp(time::sysTime()) {
	system = SysInfo::make();
	if (auto engine = Services::locate<Engine>(false)) {
		auto const& stats = engine->stats();
		upTime = stats.upTime;
		frameCount = stats.frame.count;
		framerate = stats.frame.rate;
		windowSize = engine->windowSize();
		framebufferSize = engine->framebufferSize();
	}
}

std::string ErrInfo::logMsg() const { return fmt::format("Error: {} [{}:{}] [{}]", message, source.file, source.line, source.function); }

std::string ErrInfo::json() const {
	dj::json json;
	add(json, "build", build.toString(true));
	add(json, "timestamp", time::format(timestamp, "{:%a %F %T %Z}"));
	add(json, "up_time", time::format(upTime));
	add(json, "frame_count", frameCount);
	add(json, "framerate", framerate);
	add(json, "window_size", fmt::format("{}x{}", windowSize.x, windowSize.y));
	add(json, "framebuffer_size", fmt::format("{}x{}", framebufferSize.x, framebufferSize.y));
	add(json, "message", message);
	dj::json src;
	add(src, "function", source.function);
	add(src, "file", source.file);
	add(src, "line", source.line);
	json.insert("source", std::move(src));
	dj::json sys;
	add(sys, "cpu_id", system.cpuID);
	add(sys, "gpu", system.gpuName);
	add(sys, "thread_count", system.threadCount);
	add(sys, "display_count", system.displayCount);
	json.insert("system", std::move(sys));
	dj::serial_opts_t opts;
	opts.pretty = opts.sort_keys = true;
	return json.to_string(opts);
}

bool ErrInfo::writeToFile(io::Path const& path) const {
	if (auto file = std::ofstream(path.string())) {
		file << json() << '\n';
		return true;
	}
	return false;
}

OnError const* g_onEnsureFail{};

void ErrorHandler::operator()(std::string_view message, SrcInfo const& source) const {
	ErrInfo error(std::string(message), source);
	if (error.writeToFile(path)) { logI("Error saved to [{}]", path.generic_string()); }
}

bool ErrorHandler::fileExists() const { return stdfs::exists(path.generic_string()); }
bool ErrorHandler::deleteFile() const { return stdfs::remove(path.generic_string()); }
} // namespace le::utils
