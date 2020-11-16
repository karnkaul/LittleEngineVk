#pragma once
#include <optional>
#include <core/io/path.hpp>

namespace le::io {

///
/// \brief RAII wrapper for file logging
///
struct Service final {
	Service(std::optional<Path> logFilePath);
	~Service();
};
} // namespace le::io
