#include <fstream>
#include <core/io/fs_media.hpp>
#include <core/log.hpp>
#include <core/utils/string.hpp>

namespace le::io {
std::optional<Path> FSMedia::findUpwards(Path const& leaf, Span<Path const> anyOf, u8 maxHeight) {
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

Path FSMedia::fullPath(Path const& uri) const {
	if (auto path = findPrefixed(uri)) { return io::absolute(*path); }
	return uri;
}

bool FSMedia::mount(Path path) {
	auto const pathStr = path.generic_string();
	if (std::find(m_dirs.begin(), m_dirs.end(), path) == m_dirs.end()) {
		if (!io::is_directory(path)) {
			logE("[{}] directory not found on {} [{}]!", utils::tName<FSMedia>(), info_v.name, pathStr);
			return false;
		}
		logD("[{}] directory mounted [{}]", utils::tName<FSMedia>(), pathStr);
		m_dirs.push_back(std::move(path));
		return true;
	}
	logD("[{}] directory already mounted [{}]", utils::tName<FSMedia>(), pathStr);
	return true;
}

bool FSMedia::unmount(Path const& path) noexcept {
	if (std::erase_if(m_dirs, [&path](Path const& p) { return p == path; }) > 0) {
		logD("[{}] directory umounted [{}]", utils::tName<FSMedia>(), path.generic_string());
		return true;
	}
	return false;
}

void FSMedia::clear() noexcept { m_dirs.clear(); }

std::optional<bytearray> FSMedia::bytes(Path const& uri) const {
	if (auto path = findPrefixed(uri)) {
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

std::optional<std::stringstream> FSMedia::sstream(Path const& uri) const {
	if (auto path = findPrefixed(uri)) {
		std::ifstream file(path->generic_string());
		if (file.good()) {
			std::stringstream buf;
			buf << file.rdbuf();
			return buf;
		}
	}
	return std::nullopt;
}

bool FSMedia::write(Path const& path, std::string_view str, bool newline) const {
	if (auto f = std::ofstream(path.string())) {
		f << str;
		if (newline) { f << '\n'; }
		return true;
	}
	return false;
}

bool FSMedia::write(Path const& path, Span<std::byte const> bytes) const {
	if (auto f = std::ofstream(path.string(), std::ios::binary)) {
		for (std::byte const byte : bytes) { f << static_cast<u8>(byte); }
		return true;
	}
	return false;
}

std::optional<Path> FSMedia::findPrefixed(Path const& uri) const {
	if (uri.has_root_directory() || m_dirs.empty()) { return uri; }
	for (auto const& prefix : m_dirs) {
		auto path = prefix / uri;
		if (io::is_regular_file(io::absolute(path))) { return path; }
	}
	return std::nullopt;
}
} // namespace le::io
