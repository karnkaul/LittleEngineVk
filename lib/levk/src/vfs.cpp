#include <djson/json.hpp>
#include <levk/core/c_string.hpp>
#include <levk/vfs.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace levk {
namespace fs = std::filesystem;

namespace {
template <typename Type>
auto read_data(Type& out, fs::path const& path) -> bool {
	auto file = std::ifstream{path.c_str(), std::ios::ate | std::ios::binary};
	if (!file) { return false; }
	auto const size = file.tellg();
	file.seekg({}, std::ios::beg);
	out.resize(static_cast<size_t>(size));
	file.read(reinterpret_cast<char*>(out.data()), size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	return true;
}
} // namespace

auto IVfs::load_string(std::string_view const uri) const -> std::string {
	auto const bytes = load_bytes(uri);
	if (bytes.empty()) { return {}; }
	auto ret = std::string(bytes.size(), '\0');
	std::memcpy(ret.data(), bytes.data(), ret.size());
	return ret;
}

auto IVfs::load_json(std::string_view const uri) const -> dj::Json {
	auto str = load_string(uri);
	if (str.empty()) { return {}; }
	return dj::Json::parse(str);
}

VfsFiles::VfsFiles(std::string_view mount_point) {
	if (mount_point.empty()) { mount_point = "."; }
	if (!set_mount_point(mount_point)) {
		m_mount_point = fs::current_path().generic_string();
		m_log.warn("failed to mount '{}', using working directory instead: '{}'", mount_point, mount_point);
	}
}

auto VfsFiles::upfind(std::string_view const uri, std::string_view const base_dir, bool const include_uri) -> std::string {
	for (auto dir = fs::absolute(base_dir); !dir.empty() && dir.parent_path() != dir; dir = dir.parent_path()) {
		auto const path = dir / uri;
		if (fs::exists(path)) {
			if (include_uri) { return fs::canonical(path).generic_string(); }
			return fs::canonical(dir).generic_string();
		}
	}
	return {};
}

auto VfsFiles::load_bytes(std::string_view const uri) const -> std::vector<std::byte> {
	if (uri.empty()) { return {}; }
	auto ret = std::vector<std::byte>{};
	if (!read_data(ret, fs::path{m_mount_point} / uri)) { return {}; }
	return ret;
}

auto VfsFiles::load_string(std::string_view uri) const -> std::string {
	if (uri.empty()) { return {}; }
	auto ret = std::string{};
	if (!read_data(ret, fs::path{m_mount_point} / uri)) { return {}; }
	return ret;
}

auto VfsFiles::set_mount_point(std::string_view const directory) -> bool {
	auto const path = fs::canonical(directory);
	if (path.empty()) { return false; }
	m_mount_point = path.generic_string();
	m_log.info("mounted '{}'", m_mount_point);
	return true;
}

auto VfsFiles::to_uri(std::string_view const path) const -> std::string {
	auto ret = fs::path{path}.lexically_relative(m_mount_point).generic_string();
	if (ret.empty() || ret.starts_with(".")) { return {}; }
	return ret;
}
} // namespace levk
