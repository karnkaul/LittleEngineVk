#pragma once
#include <spaced/vfs/reader.hpp>

namespace spaced {
class FileReader : public Reader {
  public:
	[[nodiscard]] static auto find_super_directory(std::string_view suffix, std::string_view start_path) -> std::string;
	[[nodiscard]] static auto write_file(char const* path, std::span<std::uint8_t const> bytes, bool overwrite) -> bool;

	explicit FileReader(std::string mount_point = ".") : m_mount_point(std::move(mount_point)) {}

	[[nodiscard]] auto read_bytes(Uri const& uri) -> std::vector<std::uint8_t> override;
	[[nodiscard]] auto read_string(Uri const& uri) -> std::string override;

	[[nodiscard]] auto write_to(Uri const& uri, std::span<std::uint8_t const> bytes, bool overwrite = true) const -> bool;

	[[nodiscard]] auto mount_point() const -> std::string_view { return m_mount_point; }

  private:
	std::string m_mount_point{};
};
} // namespace spaced
