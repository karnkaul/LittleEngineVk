#pragma once
#include <string>
#include <core/reader.hpp>
#include <core/zero.hpp>
#include <engine/resources/resource_types.hpp>

#if defined(LEVK_RESOURCES_HOT_RELOAD)
#include <functional>
#include <unordered_set>
#include <core/reader.hpp>
#include <core/ref.hpp>
#include <core/time.hpp>

namespace le::res {
class Monitor final {
  public:
	// encapsulates file-level reload logic
	struct File;

	// add files to track here (in derived constructor)
	std::vector<File> m_files;
	// time to wait before triggering reload
	Time_ms m_reloadDelay = 10ms;

  private:
	std::unordered_set<File const*> m_modified;
	time::Point m_reloadStart;
	Time_s m_reloadWait;
	u8 m_reloadTries = 3;
	u8 m_reloadFails = 0;

  public:
	stdfs::path m_id;

  public:
	// Returns true on reloaded
	bool update();
};

struct Monitor::File final {
	io::FileMonitor monitor;
	// file-level callback, invoked when modified, aborts reload on receiving false
	std::function<bool(Ref<File const>)> onModified;
	// Resource ID
	stdfs::path id;

	File(stdfs::path const& id, stdfs::path const& fullPath, io::FileMonitor::Mode mode, std::function<bool(Ref<File const>)> onModified);
};
} // namespace le::res
#endif
