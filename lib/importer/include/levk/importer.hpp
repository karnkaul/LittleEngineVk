#pragma once
#include <djson/json.hpp>
#include <levk/assets/imported_scene_asset.hpp>
#include <levk/assets/mesh_asset.hpp>
#include <levk/logger.hpp>
#include <levk/node_tree.hpp>
#include <memory>
#include <string>
#include <vector>

namespace levk {
struct ImportNode {
	ImportIndex index{};
	std::string name{};

	std::vector<ImportIndex> children{};
	ImportIndex parent{ImportIndex::eNone};

	ImportIndex mesh{ImportIndex::eNone};
	ImportIndex skeleton{ImportIndex::eNone};
	ImportIndex camera{ImportIndex::eNone};
};

struct ImportScene {
	ImportIndex index{};
	std::vector<ImportIndex> root_nodes{};
};

struct ImportAsset {
	std::vector<ImportNode> nodes{};
	std::vector<ImportScene> scenes{};
};

class Importer {
  public:
	class Builder;

	struct Shader {
		std::string_view lit_vertex_uri{};
		std::string_view skinned_vertex_uri{};
		std::string_view tbn_vertex_uri{};
		std::string_view lit_fragment_uri{};
		std::string_view tbn_fragment_uri{};
	};

	[[nodiscard]] auto get_import_asset() const -> ImportAsset const&;

	[[nodiscard]] auto get_mesh_name(ImportIndex index) const -> std::string_view;
	[[nodiscard]] auto get_skeleton_name(ImportIndex index) const -> std::string_view;

	auto import_node(ImportIndex index) -> dj::Json;
	auto import_scene(ImportIndex index) -> std::string; // TODO

  private:
	class Impl;
	struct Deleter {
		void operator()(Impl* ptr) const noexcept;
	};

	explicit Importer() = default;

	std::unique_ptr<Impl, Deleter> m_impl{};
};

class Importer::Builder {
  public:
	explicit Builder(std::string_view mount_point, Shader shader) noexcept(false);

	auto set_mount_point(std::string_view directory) -> bool;

	[[nodiscard]] auto build(char const* path) const noexcept(false) -> Importer;

	std::string import_root_dir{};
	bool force_import{false};

  private:
	[[nodiscard]] auto load_gltf(char const* path, Importer& out) const -> bool;

	Logger m_log{"Importer"};

	std::string m_mount_point{};
	Shader m_shader{};
};
} // namespace levk
