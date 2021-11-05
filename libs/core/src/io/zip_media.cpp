#include <physfs.h>
#include <core/io/fs_media.hpp>
#include <core/io/zip_media.hpp>
#include <core/log.hpp>
#include <core/services.hpp>
#include <core/utils/string.hpp>

namespace le::io {
namespace {
struct Physfs {
	bool init_{};

	bool init() {
		if (!init_) {
			if (PHYSFS_init(os::environment().arg0.data()) != 0) {
				logI("[io] ZIPFS initialized");
				init_ = true;
			} else {
				logE("[io] Failed to initialize ZIPFS!");
			}
		}
		return init_;
	}

	bool deinit() {
		if (init_) {
			init_ = false;
			if (!PHYSFS_deinit()) {
				logE("[io] ZIPFS deinit failure!");
				return false;
			}
			logD("[io] ZIPFS deinitialized");
		}
		return !init_;
	}
};

Physfs g_physfs;
} // namespace

ZIPMedia::Zip::~Zip() {
	if (g_physfs.init_ && !path.empty()) { PHYSFS_unmount(path.generic_string().data()); }
}

void ZIPMedia::Zip::exchg(Zip& lhs, Zip& rhs) noexcept { std::swap(lhs.path, rhs.path); }

bool ZIPMedia::fsInit() { return g_physfs.init(); }
bool ZIPMedia::fsDeinit() { return g_physfs.deinit(); }
bool ZIPMedia::fsActive() noexcept { return g_physfs.init_; }

ZIPMedia::ZIPMedia() { fsInit(); }

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
} // namespace le::io
