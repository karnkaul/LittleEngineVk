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

IOReader::Result<bytearray> IOReader::FBytes::operator()(stdfs::path const& id) const
{
	return pReader->getBytes(pReader->m_prefix / id);
}

IOReader::Result<std::stringstream> IOReader::FStr::operator()(stdfs::path const& id) const
{
	return pReader->getStr(pReader->m_prefix / id);
}

IOReader::IOReader(stdfs::path prefix) noexcept : m_prefix(std::move(prefix)), m_medium("Undefined")
{
	m_getBytes.pReader = this;
	m_getStr.pReader = this;
}

IOReader::IOReader(IOReader&&) noexcept = default;
IOReader& IOReader::operator=(IOReader&&) noexcept = default;
IOReader::IOReader(IOReader const&) = default;
IOReader& IOReader::operator=(IOReader const&) = default;
IOReader::~IOReader() = default;

IOReader::Result<std::string> IOReader::getString(stdfs::path const& id) const
{
	auto [result, str] = getStr(id);
	return {result, str.str()};
}

IOReader::FBytes IOReader::bytesFunctor() const
{
	return m_getBytes;
}

IOReader::FStr IOReader::strFunctor() const
{
	return m_getStr;
}

std::string_view IOReader::medium() const
{
	return m_medium;
}

bool IOReader::checkPresence(stdfs::path const& id) const
{
	if (!isPresent(id))
	{
		LOG_E("!! [{}] not found in {}!", id.generic_string(), m_medium);
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

bool IOReader::checkPresences(Span<stdfs::path const> ids) const
{
	bool bRet = true;
	for (size_t idx = 0; idx < ids.extent; ++idx)
	{
		bRet &= checkPresence(*(ids.pData + idx));
	}
	return bRet;
}

IOReader::Result<stdfs::path> FileReader::findUpwards(stdfs::path const& leaf, std::initializer_list<stdfs::path> anyOf, u8 maxHeight)
{
	for (auto const& name : anyOf)
	{
		if (stdfs::is_directory(leaf / name) || stdfs::is_regular_file(leaf / name))
		{
			auto ret = leaf.filename() == "." ? leaf.parent_path() : leaf;
			return success<stdfs::path>(std::move(ret));
		}
	}
	bool bEnd = leaf.empty() || !leaf.has_parent_path() || leaf == leaf.parent_path() || maxHeight == 0;
	if (bEnd)
	{
		return notFound<stdfs::path>();
	}
	return findUpwards(leaf.parent_path(), anyOf, maxHeight - 1);
}

IOReader::Result<stdfs::path> FileReader::findUpwards(stdfs::path const& leaf, Span<stdfs::path const> anyOf, u8 maxHeight)
{
	for (size_t idx = 0; idx < anyOf.extent; ++idx)
	{
		auto const& name = *(anyOf.pData + idx);
		if (stdfs::is_directory(leaf / name) || stdfs::is_regular_file(leaf / name))
		{
			auto ret = leaf;
			return success<stdfs::path>(std::move(ret));
		}
	}
	bool bEnd = leaf.empty() || !leaf.has_parent_path() || leaf == leaf.parent_path() || maxHeight == 0;
	if (bEnd)
	{
		return notFound<stdfs::path>();
	}
	return findUpwards(leaf.parent_path(), anyOf, maxHeight - 1);
}

FileReader::FileReader(stdfs::path prefix) noexcept : IOReader(std::move(prefix))
{
	m_medium = "Filesystem (";
	m_medium += std::move(m_prefix.generic_string());
	m_medium += ")";
	LOG_D("[{}] Filesystem mounted, idPrefix: [{}]", utils::tName(*this), m_prefix.generic_string());
}

bool FileReader::isPresent(stdfs::path const& id) const
{
	return stdfs::is_regular_file(m_prefix / id);
}

IOReader::Result<std::stringstream> FileReader::getStr(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		std::ifstream file(m_prefix / id);
		if (file.good())
		{
			std::stringstream buf;
			buf << file.rdbuf();
			return success<std::stringstream>(std::move(buf));
		}
	}
	return notFound<std::stringstream>();
}

IOReader::Result<bytearray> FileReader::getBytes(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		std::ifstream file(m_prefix / id, std::ios::binary | std::ios::ate);
		if (file.good())
		{
			auto pos = file.tellg();
			auto buf = bytearray((size_t)pos);
			file.seekg(0, std::ios::beg);
			file.read((char*)buf.data(), (std::streamsize)pos);
			return success<bytearray>(std::move(buf));
		}
	}
	return notFound<bytearray>();
}

ZIPReader::ZIPReader(stdfs::path zipPath, stdfs::path idPrefix /* = "" */) : IOReader(std::move(idPrefix)), m_zipPath(std::move(zipPath))
{
	ioImpl::initPhysfs();
	m_medium = "ZIP (";
	m_medium += std::move(m_zipPath.generic_string());
	m_medium += ")";
	if (!stdfs::is_regular_file(m_zipPath))
	{
		LOG_E("[{}] [{}] not found on Filesystem!", utils::tName<ZIPReader>(), m_zipPath.generic_string());
	}
	else
	{
		PHYSFS_mount(m_zipPath.string().data(), nullptr, 0);
		LOG_D("[{}] [{}] archive mounted, idPrefix: [{}]", utils::tName<ZIPReader>(), m_zipPath.generic_string(), m_prefix.generic_string());
	}
}

bool ZIPReader::isPresent(stdfs::path const& id) const
{
	return PHYSFS_exists((m_prefix / id).generic_string().data()) != 0;
}

IOReader::Result<std::stringstream> ZIPReader::getStr(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		auto pFile = PHYSFS_openRead((m_prefix / id).generic_string().data());
		if (pFile)
		{
			std::stringstream buf;
			auto length = PHYSFS_fileLength(pFile);
			std::string charBuf((size_t)length, 0);
			PHYSFS_readBytes(pFile, charBuf.data(), (PHYSFS_uint64)length);
			buf << charBuf;
			return success<std::stringstream>(std::move(buf));
		}
		PHYSFS_close(pFile);
	}
	return notFound<std::stringstream>();
}

IOReader::Result<bytearray> ZIPReader::getBytes(stdfs::path const& id) const
{
	if (checkPresence(id))
	{
		auto pFile = PHYSFS_openRead((m_prefix / id).generic_string().data());
		if (pFile)
		{
			auto length = PHYSFS_fileLength(pFile);
			auto buf = bytearray((size_t)length);
			PHYSFS_readBytes(pFile, buf.data(), (PHYSFS_uint64)length);
			return success<bytearray>(std::move(buf));
		}
		PHYSFS_close(pFile);
	}
	return notFound<bytearray>();
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
} // namespace le
