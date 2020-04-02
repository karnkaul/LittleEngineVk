#include <fmt/format.h>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "resource.hpp"

namespace le::gfx
{
namespace
{
Resource::GUID g_nextGUID = 0;
}

#if defined(LEVK_ASSET_HOT_RELOAD)
Resource::File::File(stdfs::path const& id, stdfs::path const& fullPath, FileMonitor::Mode mode, std::any data)
	: monitor(fullPath, mode), id(id), data(data)
{
}
#endif

Resource::Resource() : m_guid(++g_nextGUID.handle) {}

Resource::~Resource()
{
	LOG_I("-- [{}] [{}] destroyed", m_tName, m_id);
}

void Resource::setup(std::string id)
{
	m_id = std::move(id);
	m_tName = fmt::format("{} [{}]", utils::tName(*this), m_guid);
	LOG_I("== [{}] [{}] setup", m_tName, m_id);
}

Resource::Status Resource::currentStatus() const
{
	return m_status;
}
} // namespace le::gfx
