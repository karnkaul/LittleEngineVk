#pragma once
#include <gltf2cpp/gltf2cpp.hpp>
#include <filesystem>

namespace le::importer {
namespace fs = std::filesystem;

struct Input {
	fs::path gltf_path{};
	fs::path uri_prefix{};
	fs::path data_root{fs::current_path()};
	std::vector<std::size_t> meshes{};
	bool verbose{};
	bool force{};
	bool list{};
};

class Importer {
  public:
	auto run(Input input) -> bool;

  private:
	struct MeshList {
		std::vector<std::size_t> static_meshes{};
		std::vector<std::size_t> skinned_meshes{};

		static auto build(gltf2cpp::Root const& root) -> MeshList;

		[[nodiscard]] auto is_static(std::size_t const index) const { return std::ranges::find(static_meshes, index) != static_meshes.end(); }
		[[nodiscard]] auto is_skinned(std::size_t const index) const { return std::ranges::find(skinned_meshes, index) != skinned_meshes.end(); }
	};

	auto print_assets() const -> void;

	Input m_input{};
	gltf2cpp::Root m_root{};
	fs::path m_gltf_dir{};
	fs::path m_export_prefix{};
	MeshList m_mesh_list{};
};
} // namespace le::importer
