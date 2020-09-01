#include <filesystem>
#include <fstream>
#include <optional>
#include <physfs/physfs.h>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/reader.hpp>
#include <core/os.hpp>
#include <core/utils.hpp>
#include <io_impl.hpp>

namespace le::io
{
namespace
{
struct PhysfsHandle final
{
	bool bInit = false;

	PhysfsHandle();
	~PhysfsHandle();
};

PhysfsHandle::PhysfsHandle()
{
	if (PHYSFS_init(os::argv0().data()) != 0)
	{
		bInit = true;
		LOG_I("[le::io] PhysFS initialised");
	}
	else
	{
		LOG_E("Failed to initialise PhysFS!");
	}
}

PhysfsHandle::~PhysfsHandle()
{
	if (bInit)
	{
		PHYSFS_deinit();
		LOG_I("[le::io] PhysFS deinitialised");
	}
}

std::optional<PhysfsHandle> g_physfsHandle;
} // namespace

Reader::Reader() noexcept = default;
Reader::Reader(Reader&&) noexcept = default;
Reader& Reader::operator=(Reader&&) noexcept = default;
Reader::Reader(Reader const&) = default;
Reader& Reader::operator=(Reader const&) = default;
Reader::~Reader() = default;

TResult<std::string> Reader::string(stdfs::path const& id) const
{
	auto str = sstream(id);
	return {str->str(), str};
}

bool Reader::isPresent(const stdfs::path& id) const
{
	return findPrefixed(id).bResult;
}

bool Reader::checkPresence(stdfs::path const& id) const
{
	if (!isPresent(id))
	{
		LOG_E("[{}] [{}] not found in {}!", utils::tName(*this), id.generic_string(), m_medium);
		return false;
	}
	return true;
}

bool Reader::checkPresences(Span<stdfs::path> ids) const
{
	bool bRet = true;
	for (auto const& id : ids)
	{
		bRet &= checkPresence(id);
	}
	return bRet;
}

bool Reader::checkPresences(std::initializer_list<stdfs::path> ids) const
{
	return checkPresences(Span<stdfs::path>(ids));
}

std::string_view Reader::medium() const
{
	return m_medium;
}

TResult<stdfs::path> FileReader::findUpwards(stdfs::path const& leaf, Span<stdfs::path> anyOf, u8 maxHeight)
{
	for (auto const& name : anyOf)
	{
		if (stdfs::is_directory(leaf / name) || stdfs::is_regular_file(leaf / name))
		{
			auto ret = leaf.filename() == "." ? leaf.parent_path() : leaf;
			return ret / name;
		}
	}
	bool bEnd = leaf.empty() || !leaf.has_parent_path() || leaf == leaf.parent_path() || maxHeight == 0;
	if (bEnd)
	{
		return {};
	}
	return findUpwards(leaf.parent_path(), anyOf, maxHeight - 1);
}

TResult<stdfs::path> FileReader::findUpwards(stdfs::path const& leaf, std::initializer_list<stdfs::path> anyOf, u8 maxHeight)
{
	return findUpwards(leaf.parent_path(), Span<stdfs::path>(anyOf), maxHeight - 1);
}

FileReader::FileReader() noexcept
{
	m_medium = "Filesystem";
}

bool FileReader::mount(stdfs::path path)
{
	auto const pathStr = path.generic_string();
	if (std::find(m_dirs.begin(), m_dirs.end(), path) == m_dirs.end())
	{
		if (!stdfs::is_directory(path))
		{
			LOG_E("[{}] [{}] not found on Filesystem!", utils::tName<FileReader>(), pathStr);
			return false;
		}
		LOG_D("[{}] [{}] directory mounted", utils::tName<FileReader>(), pathStr);
		m_dirs.push_back(std::move(path));
		return true;
	}
	LOG_W("[{}] [{}] directory already mounted", utils::tName<FileReader>(), pathStr);
	return false;
}

TResult<bytearray> FileReader::bytes(stdfs::path const& id) const
{
	if (auto path = findPrefixed(id))
	{
		std::ifstream file(std::move(*path), std::ios::binary | std::ios::ate);
		if (file.good())
		{
			auto pos = file.tellg();
			auto buf = bytearray((std::size_t)pos);
			file.seekg(0, std::ios::beg);
			file.read((char*)buf.data(), (std::streamsize)pos);
			return buf;
		}
	}
	return {};
}

TResult<std::stringstream> FileReader::sstream(stdfs::path const& id) const
{
	if (auto path = findPrefixed(id))
	{
		std::ifstream file(std::move(*path));
		if (file.good())
		{
			std::stringstream buf;
			buf << file.rdbuf();
			return buf;
		}
	}
	return {};
}

TResult<stdfs::path> FileReader::findPrefixed(stdfs::path const& id) const
{
	auto const paths = finalPaths(id);
	for (auto const& path : paths)
	{
		if (stdfs::is_regular_file(path))
		{
			return stdfs::path(path);
		}
	}
	return {};
}

std::vector<stdfs::path> FileReader::finalPaths(stdfs::path const& id) const
{
	if (id.has_root_directory())
	{
		return {id};
	}
	std::vector<stdfs::path> ret;
	ret.reserve(m_dirs.size());
	for (auto const& prefix : m_dirs)
	{
		ret.push_back(prefix / id);
	}
	return ret;
}

stdfs::path FileReader::fullPath(stdfs::path const& id) const
{
	if (auto path = findPrefixed(id))
	{
		return stdfs::absolute(*path);
	}
	return id;
}

ZIPReader::ZIPReader()
{
	m_medium = "ZIP";
}

bool ZIPReader::mount(stdfs::path path)
{
	impl::initPhysfs();
	auto pathStr = path.generic_string();
	if (std::find(m_zips.begin(), m_zips.end(), path) == m_zips.end())
	{
		if (!stdfs::is_regular_file(path))
		{
			LOG_E("[{}] [{}] not found on Filesystem!", utils::tName<ZIPReader>(), pathStr);
			return false;
		}
		PHYSFS_mount(path.string().data(), nullptr, 0);
		LOG_D("[{}] [{}] archive mounted", utils::tName<ZIPReader>(), pathStr);
		m_zips.push_back(std::move(path));
		return true;
	}
	LOG_W("[{}] [{}] archive already mounted", utils::tName<FileReader>(), pathStr);
	return false;
}

TResult<stdfs::path> ZIPReader::findPrefixed(stdfs::path const& id) const
{
	if (PHYSFS_exists(id.generic_string().data()) != 0)
	{
		return stdfs::path(id);
	}
	return {};
}

TResult<std::stringstream> ZIPReader::sstream(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		auto pFile = PHYSFS_openRead(id.generic_string().data());
		if (pFile)
		{
			std::stringstream buf;
			auto length = PHYSFS_fileLength(pFile);
			std::string charBuf((std::size_t)length, 0);
			PHYSFS_readBytes(pFile, charBuf.data(), (PHYSFS_uint64)length);
			buf << charBuf;
			return buf;
		}
		PHYSFS_close(pFile);
	}
	return {};
}

TResult<bytearray> ZIPReader::bytes(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		auto pFile = PHYSFS_openRead(id.generic_string().data());
		if (pFile)
		{
			auto length = PHYSFS_fileLength(pFile);
			auto buf = bytearray((std::size_t)length);
			PHYSFS_readBytes(pFile, buf.data(), (PHYSFS_uint64)length);
			return buf;
		}
		PHYSFS_close(pFile);
	}
	return {};
}

void impl::initPhysfs()
{
	if (!g_physfsHandle)
	{
		g_physfsHandle = PhysfsHandle();
	}
}

void impl::deinitPhysfs()
{
	g_physfsHandle.reset();
}

FileMonitor::FileMonitor(stdfs::path const& path, Mode mode) : m_path(path), m_mode(mode)
{
	update();
}

FileMonitor::FileMonitor(FileMonitor&&) = default;
FileMonitor& FileMonitor::operator=(FileMonitor&&) = default;
FileMonitor::~FileMonitor() = default;

FileMonitor::Status FileMonitor::update()
{
	std::error_code errCode;
	if (stdfs::is_regular_file(m_path, errCode))
	{
		auto const lastWriteTime = stdfs::last_write_time(m_path, errCode);
		if (errCode)
		{
			return m_status;
		}
		if (lastWriteTime != m_lastWriteTime || m_status == Status::eNotFound)
		{
			bool bDirty = m_lastWriteTime != stdfs::file_time_type();
			m_lastWriteTime = lastWriteTime;
			if (m_mode == Mode::eTextContents)
			{
				if (auto text = s_reader.string(m_path))
				{
					if (*text == m_text)
					{
						bDirty = false;
					}
					else
					{
						m_text = std::move(*text);
						m_lastModifiedTime = m_lastWriteTime;
					}
				}
			}
			else if (m_mode == Mode::eBinaryContents)
			{
				if (auto bytes = s_reader.bytes(m_path))
				{
					if (*bytes == m_bytes)
					{
						bDirty = false;
					}
					else
					{
						m_bytes = std::move(*bytes);
						m_lastModifiedTime = m_lastWriteTime;
					}
				}
			}
			m_status = bDirty ? Status::eModified : Status::eUpToDate;
		}
		else
		{
			m_status = Status::eUpToDate;
		}
	}
	else
	{
		m_status = Status::eNotFound;
	}
	return m_status;
}

FileMonitor::Status FileMonitor::lastStatus() const
{
	return m_status;
}

stdfs::file_time_type FileMonitor::lastWriteTime() const
{
	return m_lastWriteTime;
}

stdfs::file_time_type FileMonitor::lastModifiedTime() const
{
	return m_lastModifiedTime;
}

stdfs::path const& FileMonitor::path() const
{
	return m_path;
}

std::string_view FileMonitor::text() const
{
	ASSERT(m_mode == Mode::eTextContents, "Monitor not in Text Contents mode!");
	if (m_mode != Mode::eTextContents)
	{
		LOG_E("[{}] not monitoring file contents (only timestamp) [{}]!", utils::tName<FileReader>(), m_path.generic_string());
	}
	return m_text;
}

bytearray const& FileMonitor::bytes() const
{
	ASSERT(m_mode == Mode::eBinaryContents, "Monitor not in Text Contents mode!");
	if (m_mode != Mode::eBinaryContents)
	{
		LOG_E("[{}] not monitoring file contents (only timestamp) [{}]!", utils::tName<FileReader>(), m_path.generic_string());
	}
	return m_bytes;
}
} // namespace le::io
