#pragma once
#include <core/io/converters.hpp>
#include <core/io/path.hpp>
#include <core/std_types.hpp>
#include <core/time.hpp>
#include <core/utils/error.hpp>
#include <core/version.hpp>
#include <engine/utils/sys_info.hpp>
#include <glm/vec2.hpp>

namespace le::utils {
struct ErrInfo {
	static Version const build;

	SrcInfo source;
	SysInfo system;
	std::string message;
	glm::uvec2 windowSize{};
	glm::uvec2 framebufferSize{};

	SysTime timestamp;
	Time_s upTime{};
	u64 frameCount{};
	u32 framerate{};
	u32 logThreadID{};

	ErrInfo(std::string message, SrcInfo const& source = {});

	std::string logMsg() const;
	bool writeToFile(io::Path const& path) const;
};

struct ErrorHandler : OnError {
	io::Path path = "error.txt";

	void operator()(std::string_view message, SrcInfo const& source) const override;

	bool fileExists() const;
	bool deleteFile() const;
};
} // namespace le::utils

namespace le::io {
template <>
struct Jsonify<utils::SrcInfo> : JsonHelper {
	dj::json operator()(utils::SrcInfo const& info) const;
	// not supported
	utils::SrcInfo operator()(dj::json const&) const { return {}; }
};

template <>
struct Jsonify<utils::SysInfo> : JsonHelper {
	dj::json operator()(utils::SysInfo const& info) const;
	// not supported
	utils::SysInfo operator()(dj::json const&) const { return {}; }
};

template <>
struct Jsonify<utils::ErrInfo> : JsonHelper {
	dj::json operator()(utils::ErrInfo const& info) const;
	// not supported
	utils::ErrInfo operator()(dj::json const&) const { return {{}}; }
};
} // namespace le::io
