#pragma once
#include <levk/core/polymorphic.hpp>
#include <levk/logger.hpp>
#include <cstddef>
#include <string>
#include <vector>

namespace dj {
class Json;
}

namespace levk {
/// \brief Abstract base for Vfs.
/// Vfs loads data from a source, like the filesystem.
class IVfs : public Polymorphic {
  public:
	[[nodiscard]] virtual auto load_bytes(std::string_view uri) const -> std::vector<std::byte> = 0;
	[[nodiscard]] virtual auto load_string(std::string_view uri) const -> std::string;
	[[nodiscard]] auto load_json(std::string_view uri) const -> dj::Json;
};

/// \brief Filesystem Vfs.
class VfsFiles : public IVfs {
  public:
	explicit VfsFiles(std::string_view mount_point = ".");

	/// \brief Find a directory/file upwards.
	/// \param uri Search pattern.
	/// \param base_dir Directory to start searching from.
	/// \param include_uri Whether to include uri in the returned path.
	[[nodiscard]] static auto upfind(std::string_view uri, std::string_view base_dir = ".", bool include_uri = false) -> std::string;

	[[nodiscard]] auto load_bytes(std::string_view uri) const -> std::vector<std::byte> override;
	[[nodiscard]] auto load_string(std::string_view uri) const -> std::string override;

	/// \brief Mount a directory as the root prefix for subsequent URIs.
	auto set_mount_point(std::string_view directory) -> bool;
	/// \brief Get the mounted directory path.
	[[nodiscard]] auto get_mount_point() const -> std::string_view { return m_mount_point; }

	/// \brief Convert a path to a URI relative to the mount point.
	/// \param path Path to convert.
	/// \returns URI relative to mount point, or empty string if path lies outside the mount point.
	[[nodiscard]] auto to_uri(std::string_view path) const -> std::string;

  private:
	Logger m_log{"VfsFiles"};

	std::string m_mount_point{};
};
} // namespace levk
