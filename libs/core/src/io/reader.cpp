#include <fstream>
#include <optional>
#include <physfs/physfs.h>
#include <core/ensure.hpp>
#include <core/io/reader.hpp>
#include <core/log.hpp>
#include <core/os.hpp>
#include <core/utils/string.hpp>
#include <io_impl.hpp>

namespace le::io {
namespace {
struct PhysfsHandle final {
	bool bInit = false;

	PhysfsHandle();
	~PhysfsHandle();
};

PhysfsHandle::PhysfsHandle() {
	if (PHYSFS_init(os::environment().arg0.data()) != 0) {
		bInit = true;
		logI("[le::io] PhysFS initialised");
	} else {
		logE("Failed to initialise PhysFS!");
	}
}

PhysfsHandle::~PhysfsHandle() {
	if (bInit) {
		PHYSFS_deinit();
		logI("[le::io] PhysFS deinitialised");
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

std::optional<std::string> Reader::string(io::Path const& id) const {
	if (auto str = sstream(id)) { return str->str(); }
	return std::nullopt;
}

bool Reader::present(const io::Path& id) const { return findPrefixed(id).has_value(); }

bool Reader::checkPresence(io::Path const& id) const {
	if (!present(id)) {
		logW("[{}] [{}] not found in {}!", utils::tName(this), id.generic_string(), m_medium);
		return false;
	}
	return true;
}

bool Reader::checkPresences(Span<io::Path const> ids) const {
	bool bRet = true;
	for (auto const& id : ids) { bRet &= checkPresence(id); }
	return bRet;
}

std::string_view Reader::medium() const { return m_medium; }

std::optional<io::Path> FileReader::findUpwards([[maybe_unused]] io::Path const& leaf, [[maybe_unused]] Span<io::Path const> anyOf,
												[[maybe_unused]] u8 maxHeight) {
#if defined(LEVK_OS_ANDROID)
	return std::nullopt;
#else
	for (auto const& name : anyOf) {
		if (io::is_directory(leaf / name) || io::is_regular_file(leaf / name)) {
			auto ret = leaf.filename() == "." ? leaf.parent_path() : leaf;
			return ret / name;
		}
	}
	bool bEnd = leaf.empty() || !leaf.has_parent_path() || leaf == leaf.parent_path() || maxHeight == 0;
	if (bEnd) { return std::nullopt; }
	return findUpwards(leaf.parent_path(), anyOf, maxHeight - 1);
#endif
}

FileReader::FileReader() noexcept { m_medium = "Filesystem"; }

bool FileReader::mount([[maybe_unused]] io::Path path) {
#if defined(LEVK_OS_ANDROID)
	return false;
#else
	auto const pathStr = path.generic_string();
	if (std::find(m_dirs.begin(), m_dirs.end(), path) == m_dirs.end()) {
		if (!io::is_directory(path)) {
			logE("[{}] [{}] not found on Filesystem!", utils::tName<FileReader>(), pathStr);
			return false;
		}
		logD("[{}] [{}] directory mounted", utils::tName<FileReader>(), pathStr);
		m_dirs.push_back(std::move(path));
		return true;
	}
	logW("[{}] [{}] directory already mounted", utils::tName<FileReader>(), pathStr);
	return false;
#endif
}

std::optional<bytearray> FileReader::bytes(io::Path const& id) const {
	if (auto path = findPrefixed(id)) {
		std::ifstream file(path->generic_string(), std::ios::binary | std::ios::ate);
		if (file.good()) {
			auto pos = file.tellg();
			auto buf = bytearray((std::size_t)pos);
			file.seekg(0, std::ios::beg);
			file.read((char*)buf.data(), (std::streamsize)pos);
			return buf;
		}
	}
	return std::nullopt;
}

std::optional<std::stringstream> FileReader::sstream(io::Path const& id) const {
	if (auto path = findPrefixed(id)) {
		std::ifstream file(path->generic_string());
		if (file.good()) {
			std::stringstream buf;
			buf << file.rdbuf();
			return buf;
		}
	}
	return std::nullopt;
}

std::optional<io::Path> FileReader::findPrefixed(io::Path const& id) const {
	auto const paths = finalPaths(id);
	for (auto const& path : paths) {
		if (io::is_regular_file(path)) { return io::Path(path); }
	}
	return std::nullopt;
}

std::vector<io::Path> FileReader::finalPaths(io::Path const& id) const {
	if (id.has_root_directory()) { return {id}; }
	std::vector<io::Path> ret;
	ret.reserve(m_dirs.size());
	for (auto const& prefix : m_dirs) { ret.push_back(prefix / id); }
	return ret;
}

io::Path FileReader::fullPath(io::Path const& id) const {
	if (auto path = findPrefixed(id)) { return io::absolute(*path); }
	return id;
}

ZIPReader::ZIPReader() { m_medium = "ZIP"; }

bool ZIPReader::mount(io::Path path) {
	impl::initPhysfs();
	auto pathStr = path.generic_string();
	FileReader file;
	if (std::find(m_zips.begin(), m_zips.end(), path) == m_zips.end()) {
		auto const bytes = file.bytes(path);
		if (!bytes) {
			logE("[{}] [{}] not found on Filesystem!", utils::tName<ZIPReader>(), pathStr);
			return false;
		}
		int const result = PHYSFS_mountMemory(bytes->data(), bytes->size(), nullptr, path.string().data(), nullptr, 0);
		if (result == 0) {
			logE("[{}] [{}] failed to decompress archive!", utils::tName<ZIPReader>(), pathStr);
			return false;
		}
		PHYSFS_mount(path.string().data(), nullptr, 0);
		logD("[{}] [{}] archive mounted", utils::tName<ZIPReader>(), pathStr);
		m_zips.push_back(std::move(path));
		return true;
	}
	logW("[{}] [{}] archive already mounted", utils::tName<FileReader>(), pathStr);
	return false;
}

std::optional<io::Path> ZIPReader::findPrefixed(io::Path const& id) const {
	if (PHYSFS_exists(id.generic_string().data()) != 0) { return io::Path(id); }
	return std::nullopt;
}

std::optional<std::stringstream> ZIPReader::sstream(io::Path const& id) const {
	if (checkPresence(id)) {
		auto pFile = PHYSFS_openRead(id.generic_string().data());
		if (pFile) {
			std::stringstream buf;
			auto length = PHYSFS_fileLength(pFile);
			std::string charBuf((std::size_t)length, 0);
			PHYSFS_readBytes(pFile, charBuf.data(), (PHYSFS_uint64)length);
			buf << charBuf;
			return buf;
		}
		PHYSFS_close(pFile);
	}
	return std::nullopt;
}

std::optional<bytearray> ZIPReader::bytes(io::Path const& id) const {
	if (checkPresence(id)) {
		auto pFile = PHYSFS_openRead(id.generic_string().data());
		if (pFile) {
			auto length = PHYSFS_fileLength(pFile);
			auto buf = bytearray((std::size_t)length);
			PHYSFS_readBytes(pFile, buf.data(), (PHYSFS_uint64)length);
			return buf;
		}
		PHYSFS_close(pFile);
	}
	return std::nullopt;
}

void impl::initPhysfs() {
	if (!g_physfsHandle) { g_physfsHandle = PhysfsHandle(); }
}

void impl::deinitPhysfs() { g_physfsHandle.reset(); }
} // namespace le::io
