#include <fmt/format.h>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <engine/assets/asset.hpp>

namespace le
{
namespace
{
Asset::GUID g_nextGUID = 0;
}

#if defined(LEVK_ASSET_HOT_RELOAD)
Asset::File::File(stdfs::path const& id, stdfs::path const& fullPath, FileMonitor::Mode mode, std::function<bool(File const*)> onModified)
	: monitor(fullPath, mode), onModified(onModified), id(id)
{
}
#endif

Asset::Asset(stdfs::path id) : m_id(std::move(id)), m_guid(++g_nextGUID.handle) {}

Asset::Asset(Asset&&) = default;
Asset& Asset::operator=(Asset&&) = default;

Asset::~Asset()
{
	LOGIF_I(!m_id.empty(), "-- [{}] [{}] destroyed", m_tName, m_id.generic_string());
}

void Asset::setup()
{
	m_tName = fmt::format("{} [{}]", utils::tName(*this), m_guid);
	LOG_I("== [{}] [{}] setup", m_tName, m_id.generic_string());
}

Asset::Status Asset::update()
{
	if (m_status == Status::eLoading || m_status == Status::eMoved)
	{
		return m_status;
	}
	if (m_status == Status::eReloaded)
	{
		m_status = Status::eReady;
		// skip reload check this frame
		return m_status;
	}
#if defined(LEVK_ASSET_HOT_RELOAD)
	auto const idStr = m_id.generic_string();
	if (m_reloadStart > Time() && Time::elapsed() - m_reloadStart > m_reloadDelay)
	{
		// reload all modified files
		bool bSuccess = true;
		for (auto pModified : m_modified)
		{
			if (pModified->onModified)
			{
				if (!pModified->onModified(pModified))
				{
					// failed to load file, abort reload
					LOG_W("[{}] [{}] Failed to reload file data!", m_tName, idStr);
					bSuccess = false;
					m_status = Status::eError;
					break;
				}
			}
		}
		// clear this block
		m_modified.clear();
		m_reloadStart = {};
		if (bSuccess)
		{
			// all files loaded successfully, trigger resource re-upload
			m_status = Status::eLoading;
			onReload();
			// skip reload check this frame
			return m_status;
		}
	}
	// reload check
	for (auto& file : m_files)
	{
		auto status = file.monitor.update();
		switch (status)
		{
		default:
		case FileMonitor::Status::eUpToDate:
			break;
		case FileMonitor::Status::eNotFound:
		{
			// file may still be being written out; try increasing reload delay!
			LOG_W("[{}] [{}] Resource not ready / lost!", m_tName, idStr);
			m_status = Status::eError;
			break;
		}
		case FileMonitor::Status::eModified:
		{
			// add to tracking and reset reload timer
			m_modified.insert(&file);
			m_reloadStart = Time::elapsed();
			break;
		}
		}
	}
#endif
	return m_status;
}

Asset::Status Asset::currentStatus() const
{
	return m_status;
}

bool Asset::isReady() const
{
	return m_status == Status::eReady;
}

bool Asset::isBusy() const
{
	return m_status == Status::eLoading;
}

Asset::GUID Asset::guid() const
{
	return m_guid;
}

#if defined(LEVK_ASSET_HOT_RELOAD)
void Asset::onReload() {}
#endif
} // namespace le
