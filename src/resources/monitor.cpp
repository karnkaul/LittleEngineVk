#include <fmt/format.h>
#include <core/log.hpp>
#include <resources/monitor.hpp>

#if defined(LEVK_RESOURCES_HOT_RELOAD)
namespace le::res {
Monitor::File::File(io::Path const& id, io::Path const& fullPath, io::FileMonitor::Mode mode, std::function<bool(Ref<File const>)> onModified)
	: monitor(fullPath.generic_string(), mode), onModified(onModified), id(id) {
}

bool Monitor::update() {
	auto const idStr = m_id.generic_string();
	if (m_reloadFails < m_reloadTries && m_reloadStart > time::Point() && time::now() - m_reloadStart > m_reloadWait) {
		// reload all modified files
		bool bSuccess = true;
		for (auto pModified : m_modified) {
			if (pModified->onModified) {
				if (!pModified->onModified(*pModified)) {
					// failed to load file, abort reload
					bSuccess = false;
					if (++m_reloadFails >= m_reloadTries) {
						logE("[{}] Failed to reload file data! (Re-save to retry)", idStr);
						break;
					} else {
						m_reloadStart = time::now();
						m_reloadWait = Time_s(m_reloadWait.count() * m_reloadFails * 2.0f);
						auto const wait = time::cast<Time_ms>(m_reloadWait);
						logI("[{}] Retrying reload in [{}ms]!", idStr, wait.count());
						break;
					}
				}
			}
		}
		if (bSuccess) {
			m_modified.clear();
			m_reloadStart = {};
			m_reloadWait = {};
			m_reloadFails = 0;
			// all files loaded successfully, trigger resource re-upload
			return true;
		}
	}
	// reload check
	for (auto& file : m_files) {
		auto status = file.monitor.update();
		switch (status) {
		default:
		case io::FileMonitor::Status::eUpToDate:
			break;
		case io::FileMonitor::Status::eNotFound: {
			// file may still be being written out; try increasing reload delay!
			logE("[{}] Resource not ready / lost!", idStr);
			break;
		}
		case io::FileMonitor::Status::eModified: {
			// add to tracking and reset reload timer
			m_modified.insert(&file);
			m_reloadStart = time::now();
			if (m_reloadWait == Time_s()) {
				m_reloadWait = m_reloadDelay;
			}
			if (m_reloadFails >= m_reloadTries) {
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
