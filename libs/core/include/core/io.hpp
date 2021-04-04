#pragma once
#include <core/io/path.hpp>

namespace le::io {
///
/// \brief RAII wrapper for file logging
///
class Service final {
  public:
	Service() = default;
	Service(Path logFilePath);
	Service(Service&&) noexcept;
	Service& operator=(Service&&) noexcept;
	~Service();

  private:
	void destroy();

	bool m_bActive = false;
};
} // namespace le::io
