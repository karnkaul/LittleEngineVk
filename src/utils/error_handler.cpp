#include <filesystem>
#include <fmt/format.h>
#include <build_version.hpp>
#include <core/io/fs_media.hpp>
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
	logThreadID = dl::config::log_thread_id();
	if (auto engine = Services::find<Engine>()) {
		auto const& stats = engine->stats();
		upTime = stats.upTime;
		frameCount = stats.frame.count;
		framerate = stats.frame.rate;
		windowSize = engine->windowSize();
		framebufferSize = engine->framebufferSize();
	}
}

std::string ErrInfo::logMsg() const { return fmt::format("Error: {} [{}:{}] [{}]", message, source.file, source.line, source.function); }

bool ErrInfo::writeToFile(io::Path const& path) const {
	dj::serial_opts_t opts;
	opts.pretty = opts.sort_keys = true;
	return io::FSMedia{}.write(path, io::toJson(*this).to_string(opts));
}

OnError const* g_onEnsureFail{};

void ErrorHandler::operator()(std::string_view message, SrcInfo const& source) const {
	ErrInfo error(std::string(message), source);
	if (error.writeToFile(path)) { logI("Error saved to [{}]", path.generic_string()); }
}

bool ErrorHandler::fileExists() const { return stdfs::exists(path.generic_string()); }
bool ErrorHandler::deleteFile() const { return stdfs::remove(path.generic_string()); }
} // namespace le::utils

namespace le::io {
using namespace le::utils;

dj::json Jsonify<SrcInfo>::operator()(SrcInfo const& info) const { return build("file", info.file, "function", info.function, "line", info.line); }

dj::json Jsonify<SysInfo>::operator()(utils::SysInfo const& info) const {
	return build("cpuID", info.cpuID, "gpu_name", info.gpuName, "display_count", info.displayCount, "thread_count", info.threadCount);
}

dj::json Jsonify<ErrInfo>::operator()(utils::ErrInfo const& info) const {
	return build("thread_id", info.logThreadID, "build", info.build.toString(true), "timestamp", time::format(info.timestamp, "{:%a %F %T %Z}"), "up_time",
				 time::format(info.upTime), "frame_count", info.frameCount, "framerate", info.framerate, "window_size", info.windowSize, "framebuffer_size",
				 info.framebufferSize, "message", info.message, "system", info.system, "source", info.source);
}
} // namespace le::io
