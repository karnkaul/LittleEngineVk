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
Resource::File::File(stdfs::path const& id, stdfs::path const& fullPath, FileMonitor::Mode mode) : monitor(fullPath, mode), id(id) {}
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

Resource::Status Resource::update()
{
#if defined(LEVK_ASSET_HOT_RELOAD)
	m_fileStatus = FileMonitor::Status::eUpToDate;
	for (auto& uFile : m_files)
	{
		if (uFile)
		{
			auto const fileStatus = uFile->monitor.update();
			switch (fileStatus)
			{
			default:
				break;
			case FileMonitor::Status::eNotFound:
				m_fileStatus = fileStatus;
				m_status = Status::eError;
				return m_status;
			case FileMonitor::Status::eModified:
				m_fileStatus = fileStatus;
				break;
			}
		}
	}
#endif
	return m_status;
}

Resource::Status Resource::currentStatus() const
{
	return m_status;
}
} // namespace le::gfx
