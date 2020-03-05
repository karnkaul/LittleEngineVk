#include "core/services.hpp"

namespace le
{
Services::~Services()
{
	while (!m_services.empty())
	{
		m_services.pop_back();
	}
}
} // namespace le
