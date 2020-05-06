#pragma once
#include <any>
#include <string>
#include "core/io.hpp"
#include "core/zero.hpp"

#if defined(LEVK_DEBUG)
#if !defined(LEVK_ASSET_HOT_RELOAD)
#define LEVK_ASSET_HOT_RELOAD
#endif
#endif

namespace le
{
class Asset
{
public:
	using GUID = TZero<s64, -1>;

	enum class Status : u8
	{
		eReady,
		eLoading,
		eReloaded,
		eError,
		eMoved,
		eCOUNT_
	};

#if defined(LEVK_ASSET_HOT_RELOAD)
protected:
	struct File final
	{
		FileMonitor monitor;
		stdfs::path id;
		std::any data;

		File(stdfs::path const& id, stdfs::path const& fullPath, FileMonitor::Mode mode, std::any data);
	};

protected:
	std::vector<File> m_files;
#endif

public:
	stdfs::path m_id;
	std::string m_tName;

protected:
	GUID m_guid;
	Status m_status = Status::eReady;

public:
	explicit Asset(stdfs::path id);
	Asset(Asset&&);
	Asset& operator=(Asset&&);
	virtual ~Asset();

public:
	virtual Status update() = 0;

public:
	void setup();
	Status currentStatus() const;
	bool isReady() const;
	GUID guid() const;

private:
	friend class Resources;
};
} // namespace le
