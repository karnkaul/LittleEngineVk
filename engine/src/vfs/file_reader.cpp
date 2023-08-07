#include <le/vfs/file_reader.hpp>
#include <filesystem>
#include <fstream>

namespace le {
namespace {
namespace fs = std::filesystem;

template <typename OutT>
	requires(sizeof(std::declval<OutT>()[0]) == sizeof(char))
auto read_into(OutT& out, char const* path) -> void {
	auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
	if (!file) { return; }
	auto const size = file.tellg();
	out.resize(static_cast<std::size_t>(size));
	file.seekg({}, std::ios::beg);
	// NOLINTNEXTLINE
	file.read(reinterpret_cast<char*>(out.data()), size);
}
} // namespace

auto FileReader::find_super_directory(std::string_view suffix, std::string_view start_path) -> std::string {
	for (auto path = fs::path{start_path}; !path.empty() && path != path.parent_path(); path = path.parent_path()) {
		auto ret = path / suffix;
		if (fs::is_directory(ret)) { return ret.generic_string(); }
	}
	return {};
}

auto FileReader::write_file(char const* path, std::span<std::uint8_t const> bytes, bool overwrite) -> bool {
	if (fs::exists(path)) {
		if (!overwrite) { return false; }
		fs::remove(path);
	}
	auto file = std::ofstream{path, std::ios::binary};
	if (!file) { return false; }
	// NOLINTNEXTLINE
	file.write(reinterpret_cast<char const*>(bytes.data()), static_cast<std::streamsize>(bytes.size_bytes()));
	return true;
}

auto FileReader::read_bytes(Uri const& uri) -> std::vector<std::uint8_t> {
	auto ret = std::vector<std::uint8_t>{};
	read_into(ret, uri.absolute(m_mount_point).c_str());
	return ret;
}

auto FileReader::read_string(Uri const& uri) -> std::string {
	auto ret = std::string{};
	read_into(ret, uri.absolute(m_mount_point).c_str());
	return ret;
}

auto FileReader::write_to(Uri const& uri, std::span<std::uint8_t const> bytes, bool overwrite) const -> bool {
	return write_file(uri.absolute(m_mount_point).c_str(), bytes, overwrite);
}

auto FileReader::to_uri(std::string_view const path) const -> Uri {
	if (!fs::exists(path)) { return {}; }
	return fs::absolute(path).lexically_relative(fs::absolute(m_mount_point)).generic_string();
}
} // namespace le
