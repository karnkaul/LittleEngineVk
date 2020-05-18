#include <fstream>
#include <physfs/physfs.h>
#include "core/assert.hpp"
#include "core/io.hpp"
#include "core/log.hpp"
#include "core/os.hpp"
#include "core/utils.hpp"
#include "io_impl.hpp"

namespace le
{
namespace
{
struct PhysfsHandle final
{
	PhysfsHandle();
	~PhysfsHandle();
};

PhysfsHandle::PhysfsHandle()
{
	PHYSFS_init(os::argv0().data());
}

PhysfsHandle::~PhysfsHandle()
{
	PHYSFS_deinit();
}

std::unique_ptr<PhysfsHandle> g_uPhysfsHandle;
} // namespace

IOReader::IOReader(stdfs::path prefix) noexcept : m_prefix(std::move(prefix)), m_medium("Undefined") {}

IOReader::IOReader(IOReader&&) noexcept = default;
IOReader& IOReader::operator=(IOReader&&) noexcept = default;
IOReader::IOReader(IOReader const&) = default;
IOReader& IOReader::operator=(IOReader const&) = default;
IOReader::~IOReader() = default;

TResult<std::string> IOReader::getString(stdfs::path const& id) const
{
	auto [str, bResult] = getStr(id);
	return {str.str(), bResult};
}

bool IOReader::checkPresence(stdfs::path const& id) const
{
	if (!isPresent(id))
	{
		LOG_E("[{}] [{}] not found in {}!", utils::tName(*this), id.generic_string(), m_medium);
		return false;
	}
	return true;
}

bool IOReader::checkPresences(std::initializer_list<stdfs::path> ids) const
{
	bool bRet = true;
	for (auto const& id : ids)
	{
		bRet &= checkPresence(id);
	}
	return bRet;
}

bool IOReader::checkPresences(ArrayView<stdfs::path const> ids) const
{
	bool bRet = true;
	for (auto const& id : ids)
	{
		bRet &= checkPresence(id);
	}
	return bRet;
}

std::string_view IOReader::medium() const
{
	return m_medium;
}

stdfs::path IOReader::finalPath(stdfs::path const& id) const
{
	return id.has_root_directory() ? id : m_prefix / id;
}

TResult<stdfs::path> FileReader::findUpwards(stdfs::path const& leaf, std::initializer_list<stdfs::path> anyOf, u8 maxHeight)
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

TResult<stdfs::path> FileReader::findUpwards(stdfs::path const& leaf, ArrayView<stdfs::path const> anyOf, u8 maxHeight)
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

FileReader::FileReader(stdfs::path prefix) noexcept : IOReader(std::move(prefix))
{
	m_medium = fmt::format("Filesystem ({})", m_prefix.generic_string());
}

bool FileReader::isPresent(stdfs::path const& id) const
{
	return stdfs::is_regular_file(finalPath(id));
}

TResult<std::stringstream> FileReader::getStr(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		std::ifstream file(finalPath(id));
		if (file.good())
		{
			std::stringstream buf;
			buf << file.rdbuf();
			return buf;
		}
	}
	return {};
}

TResult<bytearray> FileReader::getBytes(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		std::ifstream file(finalPath(id), std::ios::binary | std::ios::ate);
		if (file.good())
		{
			auto pos = file.tellg();
			auto buf = bytearray((size_t)pos);
			file.seekg(0, std::ios::beg);
			file.read((char*)buf.data(), (std::streamsize)pos);
			return buf;
		}
	}
	return {};
}

stdfs::path FileReader::fullPath(stdfs::path const& id) const
{
	return stdfs::absolute(finalPath(id));
}

ZIPReader::ZIPReader(stdfs::path zipPath, stdfs::path idPrefix /* = "" */) : IOReader(std::move(idPrefix)), m_zipPath(std::move(zipPath))
{
	ioImpl::initPhysfs();
	m_medium = fmt::format("ZIP ({})", m_zipPath.generic_string());
	if (!stdfs::is_regular_file(m_zipPath))
	{
		LOG_E("[{}] [{}] not found on Filesystem!", utils::tName<ZIPReader>(), m_zipPath.generic_string());
	}
	else
	{
		PHYSFS_mount(m_zipPath.string().data(), nullptr, 0);
		LOG_D("[{}] [{}] archive mounted, idPrefix: [{}]", utils::tName<ZIPReader>(), m_zipPath.generic_string(),
			  m_prefix.generic_string());
	}
}

bool ZIPReader::isPresent(stdfs::path const& id) const
{
	return PHYSFS_exists((finalPath(id)).generic_string().data()) != 0;
}

TResult<std::stringstream> ZIPReader::getStr(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		auto pFile = PHYSFS_openRead((finalPath(id)).generic_string().data());
		if (pFile)
		{
			std::stringstream buf;
			auto length = PHYSFS_fileLength(pFile);
			std::string charBuf((size_t)length, 0);
			PHYSFS_readBytes(pFile, charBuf.data(), (PHYSFS_uint64)length);
			buf << charBuf;
			return buf;
		}
		PHYSFS_close(pFile);
	}
	return {};
}

TResult<bytearray> ZIPReader::getBytes(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		auto pFile = PHYSFS_openRead((finalPath(id)).generic_string().data());
		if (pFile)
		{
			auto length = PHYSFS_fileLength(pFile);
			auto buf = bytearray((size_t)length);
			PHYSFS_readBytes(pFile, buf.data(), (PHYSFS_uint64)length);
			return buf;
		}
		PHYSFS_close(pFile);
	}
	return {};
}

void ioImpl::initPhysfs()
{
	if (!g_uPhysfsHandle)
	{
		g_uPhysfsHandle = std::make_unique<PhysfsHandle>();
		LOG_D("PhysFS initialised");
	}
}

void ioImpl::deinitPhysfs()
{
	LOG_D("PhysFS deinitialised");
	g_uPhysfsHandle = nullptr;
}

FileReader FileMonitor::s_reader;

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
				auto [text, bResult] = s_reader.getString(m_path);
				if (bResult)
				{
					if (text == m_text)
					{
						bDirty = false;
					}
					else
					{
						m_text = std::move(text);
						m_lastModifiedTime = m_lastWriteTime;
					}
				}
			}
			else if (m_mode == Mode::eBinaryContents)
			{
				auto [bytes, bResult] = s_reader.getBytes(m_path);
				if (bResult)
				{
					if (bytes == m_bytes)
					{
						bDirty = false;
					}
					else
					{
						m_bytes = std::move(bytes);
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
} // namespace le
