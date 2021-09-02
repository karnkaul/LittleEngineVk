#include <fstream>
#include <optional>
#include <physfs/physfs.h>
#include <core/ensure.hpp>
#include <core/io/reader.hpp>
#include <core/log.hpp>
#include <core/os.hpp>
#include <core/services.hpp>
#include <core/utils/string.hpp>

namespace le::io {
std::optional<std::string> Reader::string(Path const& id) const {
	if (auto str = sstream(id)) { return str->str(); }
	return std::nullopt;
}

bool Reader::present(const Path& id) const { return findPrefixed(id).has_value(); }

bool Reader::checkPresence(Path const& id) const {
	if (!present(id)) {
		logW("[{}] [{}] not found in {}!", utils::tName(this), id.generic_string(), m_medium);
		return false;
	}
	return true;
}

bool Reader::checkPresences(Span<Path const> ids) const {
	bool ret = true;
	for (auto const& id : ids) { ret &= checkPresence(id); }
	return ret;
}

FileReader::FileReader() noexcept { m_medium = "Filesystem"; }

std::optional<Path> FileReader::findUpwards(Path const& leaf, Span<Path const> anyOf, u8 maxHeight) {
	for (auto const& name : anyOf) {
		if (io::is_directory(leaf / name) || io::is_regular_file(leaf / name)) {
			auto ret = leaf.filename() == "." ? leaf.parent_path() : leaf;
			return ret / name;
		}
	}
	bool none = leaf.empty() || !leaf.has_parent_path() || leaf == leaf.parent_path() || maxHeight == 0;
	if (none) { return std::nullopt; }
	return findUpwards(leaf.parent_path(), anyOf, maxHeight - 1);
}

bool FileReader::write(Path const& path, std::string_view str, bool newline) {
	if (auto f = std::ofstream(path.string())) {
		f << str;
		if (newline) { f << '\n'; }
		return true;
	}
	return false;
}

bool FileReader::write(Path const& path, Span<std::byte const> bytes) {
	if (auto f = std::ofstream(path.string(), std::ios::binary)) {
		for (std::byte const byte : bytes) { f << static_cast<u8>(byte); }
		return true;
	}
	return false;
}

bool FileReader::mount(Path path) {
	auto const pathStr = path.generic_string();
	if (std::find(m_dirs.begin(), m_dirs.end(), path) == m_dirs.end()) {
		if (!io::is_directory(path)) {
			logE("[{}] directory not found on {} [{}]!", utils::tName<FileReader>(), m_medium, pathStr);
			return false;
		}
		logD("[{}] directory mounted [{}]", utils::tName<FileReader>(), pathStr);
		m_dirs.push_back(std::move(path));
		return true;
	}
	logD("[{}] directory already mounted [{}]", utils::tName<FileReader>(), pathStr);
	return true;
}

bool FileReader::unmount(Path const& path) noexcept {
	if (std::erase_if(m_dirs, [&path](Path const& p) { return p == path; }) > 0) {
		logD("[{}] directory umounted [{}]", utils::tName<FileReader>(), path.generic_string());
		return true;
	}
	return false;
}

void FileReader::clear() noexcept { m_dirs.clear(); }

std::optional<bytearray> FileReader::bytes(Path const& id) const {
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

std::optional<std::stringstream> FileReader::sstream(Path const& id) const {
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

std::optional<Path> FileReader::findPrefixed(Path const& id) const {
	auto const paths = finalPaths(id);
	for (auto const& path : paths) {
		if (io::is_regular_file(path)) { return Path(path); }
	}
	return std::nullopt;
}

std::vector<Path> FileReader::finalPaths(Path const& id) const {
	if (id.has_root_directory()) { return {id}; }
	if (m_dirs.empty()) { return {id}; }
	std::vector<Path> ret;
	ret.reserve(m_dirs.size());
	for (auto const& prefix : m_dirs) { ret.push_back(prefix / id); }
	return ret;
}

Path FileReader::fullPath(Path const& id) const {
	if (auto path = findPrefixed(id)) { return io::absolute(*path); }
	return id;
}

ZIPReader::ZIPReader(not_null<ZIPFS const*> zipfs) noexcept : m_zipfs(zipfs) { m_medium = "ZIP"; }

ZIPReader& ZIPReader::operator=(ZIPReader&& rhs) noexcept {
	if (&rhs != this) {
		clear();
		Reader::operator=(std::move(rhs));
		m_zips = std::move(rhs.m_zips);
	}
	return *this;
}

ZIPReader::~ZIPReader() noexcept { clear(); }

bool ZIPReader::mount(Path path) {
	if (!*m_zipfs) { return false; }
	auto pathStr = path.generic_string();
	FileReader file;
	if (std::find(m_zips.begin(), m_zips.end(), path) == m_zips.end()) {
		auto const bytes = file.bytes(path);
		if (!bytes) {
			logE("[{}] not found on {} [{}]!", utils::tName<ZIPReader>(), file.medium(), pathStr);
			return false;
		}
		return mount(std::move(path), std::move(*bytes));
	}
	logW("[{}] [{}] archive already mounted", utils::tName<FileReader>(), pathStr);
	return false;
}

bool ZIPReader::mount(Path point, Span<std::byte const> bytes) {
	if (!*m_zipfs) { return false; }
	auto const str = point.generic_string();
	int const result = PHYSFS_mountMemory(bytes.data(), bytes.size(), nullptr, str.data(), nullptr, 0);
	if (result == 0) {
		logE("[{}] failed to decompress archive [{}]!", utils::tName<ZIPReader>(), str);
		return false;
	}
	PHYSFS_mount(str.data(), nullptr, 0);
	logD("[{}] archive mounted [{}]", utils::tName<ZIPReader>(), str);
	m_zips.push_back(std::move(point));
	return true;
}

bool ZIPReader::unmount(Path const& path) noexcept {
	if (!*m_zipfs) { return false; }
	for (auto it = m_zips.begin(); it != m_zips.end(); ++it) {
		if (*it == path) {
			auto const str = path.generic_string();
			PHYSFS_unmount(str.data());
			m_zips.erase(it);
			logD("[{}] archive unmounted [{}]", utils::tName<ZIPReader>(), str);
			return true;
		}
	}
	return false;
}

void ZIPReader::clear() noexcept {
	if (*m_zipfs) {
		for (auto const& path : m_zips) { PHYSFS_unmount(path.generic_string().data()); }
	}
	m_zips.clear();
}

std::optional<Path> ZIPReader::findPrefixed(Path const& id) const {
	if (!*m_zipfs) { return std::nullopt; }
	if (PHYSFS_exists(id.generic_string().data()) != 0) { return Path(id); }
	return std::nullopt;
}

std::optional<std::stringstream> ZIPReader::sstream(Path const& id) const {
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

std::optional<bytearray> ZIPReader::bytes(Path const& id) const {
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

ZIPFS::ZIPFS() {
	if (auto zipfs = Services::locate<ZIPFS>(false); zipfs && *zipfs) {
		logW("[io] ZIPFS already initialized");
		return;
	}
	if (PHYSFS_init(os::environment().arg0.data()) != 0) {
		logI("[io] ZIPFS initialized");
		m_init = true;
		Services::track<ZIPFS>(this);
	} else {
		logE("[io] Failed to initialize ZIPFS!");
	}
}

ZIPFS::~ZIPFS() {
	if (m_init) {
		Services::untrack<ZIPFS>();
		PHYSFS_deinit();
		logD("[io] ZIPFS deinitialized");
	}
}
} // namespace le::io
