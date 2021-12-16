#include <fmt/format.h>
#include <build_version.hpp>
#include <core/io/fs_media.hpp>
#include <core/services.hpp>
#include <core/utils/data_store.hpp>
#include <dumb_json/json.hpp>
#include <engine/engine.hpp>
#include <engine/utils/engine_stats.hpp>
#include <engine/utils/error_handler.hpp>
#include <filesystem>

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

Version const ErrList::build = g_buildVersion;

ErrInfo::ErrInfo(std::string message, SrcInfo const& source) : source(source), message(std::move(message)), timestamp(time::sysTime()) {
	logThreadID = dlog::this_thread_id();
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

void ErrorHandler::operator()(std::string_view message, SrcInfo const& source) {
	ktl::tlock(m_list.errors)->emplace_back(std::string(message), source);
	logD("Error recorded: {}", message);
}

ErrorHandler::~ErrorHandler() {
	m_list.errors.mutex.lock();
	if (!m_list.errors.t.empty()) {
		if (auto si = DataObject<SysInfo>("sys_info")) { m_list.sysInfo = *si; }
		dj::serial_opts_t opts;
		opts.sort_keys = opts.pretty = true;
		m_list.errors.mutex.unlock();
		if (io::FSMedia{}.write(m_path, io::toJson(m_list).to_string(opts))) { logI("Errors saved to [{}]", m_path.generic_string()); }
		return;
	}
	m_list.errors.mutex.unlock();
}

bool ErrorHandler::fileExists() const { return stdfs::exists(m_path.generic_string()); }
bool ErrorHandler::deleteFile() const { return stdfs::remove(m_path.generic_string()); }

} // namespace le::utils

namespace le::io {
using namespace le::utils;

dj::json Jsonify<SrcInfo>::operator()(SrcInfo const& info) const { return build("file", info.file, "function", info.function, "line", info.line); }

dj::json Jsonify<SysInfo>::operator()(utils::SysInfo const& info) const {
	return build("os", info.osName, "arch", info.arch, "cpuID", info.cpuID, "gpu_name", info.gpuName, "display_count", info.displayCount, "thread_count",
				 info.threadCount, "present_mode", info.presentMode, "swapchain_images", info.swapchainImages);
}

dj::json Jsonify<ErrInfo>::operator()(utils::ErrInfo const& info) const {
	return build("thread_id", info.logThreadID, "timestamp", time::format(info.timestamp, "{:%a %F %T %Z}"), "up_time", time::format(info.upTime),
				 "frame_count", info.frameCount, "framerate", info.framerate, "window_size", info.windowSize, "framebuffer_size", info.framebufferSize,
				 "message", info.message, "source", info.source);
}

dj::json Jsonify<ErrList>::operator()(utils::ErrList const& list) const {
	return build("build", list.build.toString(true), "system", list.sysInfo, "errors", *ktl::tlock(list.errors));
}
} // namespace le::io
