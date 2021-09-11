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
	Service(Service&& rhs) noexcept { exchg(*this, rhs); }
	Service& operator=(Service rhs) noexcept { return (exchg(*this, rhs), *this); }
	~Service();
	static void exchg(Service& lhs, Service& rhs) noexcept;

  private:
	bool m_active{};
};
} // namespace le::io
