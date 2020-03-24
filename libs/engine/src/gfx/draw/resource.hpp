#pragma once
#include <string>
#include "core/std_types.hpp"
#include "core/zero.hpp"

#if defined(LEVK_DEBUG)
#if !defined(LEVK_RESOURCES_UPDATE)
#define LEVK_RESOURCES_UPDATE
#endif
#endif

#if defined(LEVK_RESOURCES_UPDATE)
#include <memory>
#include "core/io.hpp"
#endif

namespace le::gfx
{
class Resource
{
public:
	using GUID = TZero<s64, -1>;

#if defined(LEVK_RESOURCES_UPDATE)
protected:
	struct File
	{
		FileMonitor monitor;
		stdfs::path id;

		File(stdfs::path const& id, stdfs::path const& fullPath);
		File(File&&) = default;
		File& operator=(File&&) = default;
		virtual ~File() = default;
	};

protected:
	std::vector<std::unique_ptr<File>> m_files;
	FileMonitor::Status m_lastStatus = FileMonitor::Status::eUpToDate;
#endif

public:
	std::string m_id;
	std::string m_tName;
	GUID const m_guid;

public:
	Resource();
	virtual ~Resource();

	void setup(std::string id);
#if defined(LEVK_RESOURCES_UPDATE)
	virtual FileMonitor::Status update();
	FileMonitor::Status currentStatus() const;
#endif
};
} // namespace le::gfx
