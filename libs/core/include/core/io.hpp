#pragma once
#include <optional>
#include <core/io/path.hpp>

namespace le::io {
///
/// \brief RAII wrapper for file logging
///
class Service final {
  public:
	Service() = default;
	Service(std::optional<Path> logFilePath);
	Service(Service&&);
	Service& operator=(Service&&);
	~Service();

  private:
	void destroy();

	bool m_bActive = false;
};
} // namespace le::io
