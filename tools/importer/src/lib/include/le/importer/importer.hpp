#pragma once
#include <gltf2cpp/gltf2cpp.hpp>
#include <le/vfs/uri.hpp>
#include <filesystem>

namespace le::importer {
namespace fs = std::filesystem;

struct Input {
	fs::path gltf_path{};
	fs::path uri_prefix{};
	fs::path data_root{fs::current_path()};
	bool verbose{};
	bool force{};
};

struct MeshList {
	std::vector<std::size_t> static_meshes{};
	std::vector<std::size_t> skinned_meshes{};

	[[nodiscard]] auto is_static(std::size_t const index) const { return std::ranges::find(static_meshes, index) != static_meshes.end(); }
	[[nodiscard]] auto is_skinned(std::size_t const index) const { return std::ranges::find(skinned_meshes, index) != skinned_meshes.end(); }
};

class Importer {
  public:
	auto setup(Input input) -> bool;

	auto get_mesh_list() -> MeshList const& { return m_mesh_list; }
	auto import_mesh(std::size_t mesh_id) -> std::optional<Uri>;
	auto print_assets() const -> void;

  private:
	Input m_input{};
	gltf2cpp::Root m_root{};
	fs::path m_gltf_dir{};
	fs::path m_export_prefix{};
	MeshList m_mesh_list{};
};
} // namespace le::importer
