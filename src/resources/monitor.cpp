#include <fmt/format.h>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <resources/monitor.hpp>

#if defined(LEVK_RESOURCES_HOT_RELOAD)
namespace le::res
{
Monitor::File::File(stdfs::path const& id, stdfs::path const& fullPath, io::FileMonitor::Mode mode, std::function<bool(Ref<File const>)> onModified)
	: monitor(fullPath, mode), onModified(onModified), id(id)
{
}

bool Monitor::update()
{
	auto const idStr = m_id.generic_string();
	if (m_reloadFails < m_reloadTries && m_reloadStart > Time() && Time::elapsed() - m_reloadStart > m_reloadWait)
	{
		// reload all modified files
		bool bSuccess = true;
		for (auto pModified : m_modified)
		{
			if (pModified->onModified)
			{
				if (!pModified->onModified(*pModified))
				{
					// failed to load file, abort reload
					bSuccess = false;
					if (++m_reloadFails >= m_reloadTries)
					{
						LOG_E("[{}] Failed to reload file data! (Re-save to retry)", idStr);
						break;
					}
					else
					{
						m_reloadStart = Time::elapsed();
						m_reloadWait.scale(m_reloadFails * 2.0f);
						LOG_I("[{}] Retrying reload in [{}ms]!", idStr, m_reloadWait.to_ms());
						break;
					}
				}
			}
		}
		if (bSuccess)
		{
			m_modified.clear();
			m_reloadStart = m_reloadWait = {};
			m_reloadFails = 0;
			// all files loaded successfully, trigger resource re-upload
			return true;
		}
	}
	// reload check
	for (auto& file : m_files)
	{
		auto status = file.monitor.update();
		switch (status)
		{
		default:
		case io::FileMonitor::Status::eUpToDate:
			break;
		case io::FileMonitor::Status::eNotFound:
		{
			// file may still be being written out; try increasing reload delay!
			LOG_E("[{}] Resource not ready / lost!", idStr);
			break;
		}
		case io::FileMonitor::Status::eModified:
		{
			// add to tracking and reset reload timer
			m_modified.insert(&file);
			m_reloadStart = Time::elapsed();
			if (m_reloadWait == Time())
			{
				m_reloadWait = m_reloadDelay;
			}
			if (m_reloadFails >= m_reloadTries)
			{
				m_reloadWait = m_reloadDelay;
				m_reloadFails = 0;
			}
			break;
		}
		}
	}
	return false;
}
} // namespace le::res
#endif
