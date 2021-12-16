#pragma once
#include <core/io/path.hpp>
#include <core/log.hpp>
#include <dumb_log/pipe.hpp>

namespace le::io {
///
/// \brief RAII wrapper for file logging
///
class Service final {
  public:
	Service() = default;
	Service(Path logFilePath, LogChannel active = 0xff);

  private:
	dlog::pipe::handle m_handle;
};
} // namespace le::io
