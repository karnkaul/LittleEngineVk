#pragma once
#include <dumb_log/pipe.hpp>
#include <levk/core/io/path.hpp>
#include <levk/core/log.hpp>

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
