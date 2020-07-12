#pragma once
#include <string>
#include <core/reader.hpp>
#include <core/zero.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_ASSET_HOT_RELOAD)
#define LEVK_ASSET_HOT_RELOAD
#endif
#endif

// Hot Reload will reload assets on changes to linked files (filesystem only)
// Asset::update() will track modified files and trigger reload after a
// period of time (200ms by default); and a final onReload() if successful.
#if defined(LEVK_ASSET_HOT_RELOAD)
#include <functional>
#include <unordered_set>
#include <core/time.hpp>
#endif

namespace le
{
class Asset
{
public:
	using GUID = TZero<s64, -1>;

	enum class Status : s8
	{
		eIdle,
		eReady,		// ready to use
		eLoading,	// transferring resources
		eReloading, // reloading (only relevant if `LEVK_ASSET_HOT_RELOAD` is defined)
		eReloaded,	// finished reloading (only for one frame)
		eError,		// cannot be used
		eMoved,		// cannot be used
		eCOUNT_
	};

#if defined(LEVK_ASSET_HOT_RELOAD)
protected:
	// encapsulates file-level reload logic
	struct File;

protected:
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
#endif

public:
	stdfs::path m_id;
	std::string m_tName;

protected:
	GUID m_guid;
	Status m_status = Status::eIdle;

public:
	explicit Asset(stdfs::path id);
	Asset(Asset&&);
	Asset& operator=(Asset&&);
	virtual ~Asset();

public:
	// overrides must call Asset::update() for hot reload logic
	virtual Status update();

public:
	void setup();

	Status currentStatus() const;
	bool isReady() const;
	bool isBusy() const;
	GUID guid() const;

#if defined(LEVK_ASSET_HOT_RELOAD)
protected:
	// invoked when all files have successfully been reloaded
	virtual void onReload();
#endif

private:
	friend class Resources;
};

#if defined(LEVK_ASSET_HOT_RELOAD)
struct Asset::File final
{
	io::FileMonitor monitor;
	// file-level callback, invoked when modified, aborts reload on receiving false
	std::function<bool(File const*)> onModified;
	// Asset ID
	stdfs::path id;

	File(stdfs::path const& id, stdfs::path const& fullPath, io::FileMonitor::Mode mode, std::function<bool(File const*)> onModified);
};
#endif
} // namespace le
