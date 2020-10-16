#pragma once
#include <filesystem>
#include <optional>

namespace le::io {
namespace stdfs = std::filesystem;

///
/// \brief RAII wrapper for file logging
///
struct Service final {
	Service(std::optional<stdfs::path> logFilePath);
	~Service();
};
} // namespace le::io
