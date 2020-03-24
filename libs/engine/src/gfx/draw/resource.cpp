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

#if defined(LEVK_RESOURCES_UPDATE)
Resource::File::File(stdfs::path const& id, stdfs::path const& fullPath) : monitor(fullPath, FileMonitor::Mode::eContents), id(id) {}
#endif

Resource::Resource() : m_guid(++g_nextGUID.handle) {}

Resource::~Resource()
{
	LOG_I("-- [{}] [{}] destroyed", m_tName, m_id);
}

void Resource::setup(std::string id)
{
	m_id = std::move(id);
	m_tName = fmt::format("{}{}", utils::tName(*this), m_guid);
	LOG_I("== [{}] [{}] setup", m_tName, m_id);
}

#if defined(LEVK_RESOURCES_UPDATE)
FileMonitor::Status Resource::update()
{
	m_lastStatus = FileMonitor::Status::eUpToDate;
	for (auto& uFile : m_files)
	{
		if (uFile)
		{
			auto const status = uFile->monitor.update();
			switch (status)
			{
			default:
				break;
			case FileMonitor::Status::eNotFound:
				return m_lastStatus = status;
			case FileMonitor::Status::eModified:
				m_lastStatus = status;
				break;
			}
		}
	}
	return m_lastStatus;
}

FileMonitor::Status Resource::currentStatus() const
{
	return m_lastStatus;
}
#endif
} // namespace le::gfx
