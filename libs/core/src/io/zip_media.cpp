#include <physfs.h>
#include <core/io/fs_media.hpp>
#include <core/io/zip_media.hpp>
#include <core/log.hpp>
#include <core/services.hpp>
#include <core/utils/string.hpp>

namespace le::io {
ZIPMedia::Zip::~Zip() {
	if (!path.empty()) { PHYSFS_unmount(path.generic_string().data()); }
}

void ZIPMedia::Zip::exchg(Zip& lhs, Zip& rhs) noexcept { std::swap(lhs.path, rhs.path); }

bool ZIPMedia::fsActive() noexcept {
	if (auto fs = Services::find<ZIPFS>()) { return fs->active(); }
	return false;
}

bool ZIPMedia::mount(Path path) {
	if (!fsActive()) { return false; }
	auto const str = path.generic_string();
	if (std::find(m_zips.begin(), m_zips.end(), path) == m_zips.end()) {
		auto bytes = FSMedia{}.bytes(path);
		if (!bytes) {
			logE("[{}] not found on {} [{}]!", utils::tName<ZIPMedia>(), FSMedia::info_v.name, str);
			return false;
		}
		return mount(std::move(path), std::move(*bytes));
	}
	logW("[{}] [{}] archive already mounted", utils::tName<FSMedia>(), str);
	return false;
}

bool ZIPMedia::mount(Path point, Span<std::byte const> bytes) {
	if (!fsActive()) { return false; }
	auto const str = point.generic_string();
	int const result = PHYSFS_mountMemory(bytes.data(), bytes.size(), nullptr, str.data(), nullptr, 0);
	if (result == 0) {
		logE("[{}] failed to decompress archive [{}]!", utils::tName<ZIPMedia>(), str);
		return false;
	}
	PHYSFS_mount(str.data(), nullptr, 0);
	logD("[{}] archive mounted [{}]", utils::tName<ZIPMedia>(), str);
	m_zips.push_back(std::move(point));
	return true;
}

bool ZIPMedia::unmount(Path const& path) noexcept {
	if (!fsActive()) { return false; }
	for (auto it = m_zips.begin(); it != m_zips.end(); ++it) {
		if (*it == path) {
			auto const str = path.generic_string();
			PHYSFS_unmount(str.data());
			m_zips.erase(it);
			logD("[{}] archive unmounted [{}]", utils::tName<ZIPMedia>(), str);
			return true;
		}
	}
	return false;
}

void ZIPMedia::clear() noexcept { m_zips.clear(); }

std::optional<Path> ZIPMedia::findPrefixed(Path const& uri) const {
	if (fsActive() && PHYSFS_exists(uri.generic_string().data()) != 0) { return Path(uri); }
	return std::nullopt;
}

std::optional<std::stringstream> ZIPMedia::sstream(Path const& uri) const {
	if (present(uri, dl::level::warn)) {
		if (auto file = PHYSFS_openRead(uri.generic_string().data())) {
			std::stringstream buf;
			auto length = PHYSFS_fileLength(file);
			std::string charBuf((std::size_t)length, 0);
			PHYSFS_readBytes(file, charBuf.data(), (PHYSFS_uint64)length);
			buf << charBuf;
			PHYSFS_close(file);
			return buf;
		}
	}
	return std::nullopt;
}

std::optional<bytearray> ZIPMedia::bytes(Path const& uri) const {
	if (present(uri, dl::level::warn)) {
		if (auto file = PHYSFS_openRead(uri.generic_string().data())) {
			auto length = PHYSFS_fileLength(file);
			auto buf = bytearray((std::size_t)length);
			PHYSFS_readBytes(file, buf.data(), (PHYSFS_uint64)length);
			PHYSFS_close(file);
			return buf;
		}
	}
	return std::nullopt;
}

ZIPFS::ZIPFS() {
	if (auto zipfs = Services::find<ZIPFS>(); zipfs && *zipfs) {
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
