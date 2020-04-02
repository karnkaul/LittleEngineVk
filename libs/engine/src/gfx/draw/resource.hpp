#pragma once
#include <any>
#include <string>
#include "core/std_types.hpp"
#include "core/zero.hpp"

#if defined(LEVK_DEBUG)
#if !defined(LEVK_ASSET_HOT_RELOAD)
#define LEVK_ASSET_HOT_RELOAD
#endif
#endif

#if defined(LEVK_ASSET_HOT_RELOAD)
#include "core/io.hpp"
#endif

namespace le::gfx
{
class Resource
{
public:
	using GUID = TZero<s64, -1>;

	enum class Status : u8
	{
		eReady = 0,
		eLoading,
		eReloaded,
		eError,
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
	std::string m_id;
	std::string m_tName;
	GUID const m_guid;

protected:
	Status m_status = Status::eReady;

public:
	Resource();
	virtual ~Resource();

public:
	virtual Status update() = 0;

public:
	void setup(std::string id);
	Status currentStatus() const;
};
} // namespace le::gfx
