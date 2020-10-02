#include <core/services.hpp>

namespace le {
Services::Services() = default;
Services::Services(Services&&) = default;
Services& Services::operator=(Services&&) = default;

Services::~Services() {
	while (!m_services.empty()) {
		m_services.pop_back();
	}
}
} // namespace le
