#pragma once
#include <core/io/converters.hpp>
#include <core/io/path.hpp>
#include <core/std_types.hpp>
#include <core/time.hpp>
#include <core/utils/error.hpp>
#include <core/utils/sys_info.hpp>
#include <core/version.hpp>
#include <glm/vec2.hpp>
#include <ktl/tmutex.hpp>

namespace le::utils {
struct ErrInfo {
	SrcInfo source;
	std::string message;
	glm::uvec2 windowSize{};
	glm::uvec2 framebufferSize{};

	SysTime timestamp;
	Time_s upTime{};
	u64 frameCount{};
	u32 framerate{};
	int logThreadID{};

	ErrInfo(std::string message, SrcInfo const& source = {});

	std::string logMsg() const;
	bool writeToFile(io::Path const& path) const;
};

struct ErrList {
	static Version const build;

	SysInfo sysInfo;
	ktl::tmutex<std::vector<ErrInfo>> errors;
};

class ErrorHandler : public OnError {
  public:
	ErrorHandler() = default;
	virtual ~ErrorHandler();

	void operator()(std::string_view message, SrcInfo const& source) override;

	bool fileExists() const;
	bool deleteFile() const;

	ErrList m_list;
	io::Path m_path = "error.txt";
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

template <>
struct Jsonify<utils::ErrList> : JsonHelper {
	dj::json operator()(utils::ErrList const& info) const;
	// not supported
	utils::ErrList operator()(dj::json const&) const { return {}; }
};
} // namespace le::io
