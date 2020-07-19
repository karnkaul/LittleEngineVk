#pragma once
#include <string>
#include <core/reader.hpp>
#include <core/zero.hpp>
#include <engine/resources/resource_types.hpp>

#if defined(LEVK_RESOURCES_HOT_RELOAD)
#include <functional>
#include <unordered_set>
#include <core/time.hpp>
#include <core/reader.hpp>

namespace le::resources
{
class Monitor final
{
public:
	// encapsulates file-level reload logic
	struct File;

	// add files to track here (in derived constructor)
	std::vector<File> m_files;
	// time to wait before triggering reload
	Time m_reloadDelay = 10ms;

private:
	std::unordered_set<File const*> m_modified;
	Time m_reloadStart;
	Time m_reloadWait;
	u8 m_reloadTries = 3;
	u8 m_reloadFails = 0;

public:
	stdfs::path m_id;

public:
	// Returns true on reloaded
	bool update();
};

struct Monitor::File final
{
	io::FileMonitor monitor;
	// file-level callback, invoked when modified, aborts reload on receiving false
	std::function<bool(File const*)> onModified;
	// Asset ID
	stdfs::path id;

	File(stdfs::path const& id, stdfs::path const& fullPath, io::FileMonitor::Mode mode, std::function<bool(File const*)> onModified);
};
} // namespace le::resources
#endif
