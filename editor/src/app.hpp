#pragma once
#include <asset_tree.hpp>
#include <levk/engine.hpp>
#include <levk/imported_scene.hpp>
#include <levk/importer.hpp>
#include <levk/logger.hpp>
#include <levk/materials/unlit.hpp>
#include <levk/render_context.hpp>
#include <levk/render_stats.hpp>
#include <levk/scene.hpp>
#include <loader.hpp>
#include <filesystem>

namespace fs = std::filesystem;

class App : public levk::EventSink {
  public:
	explicit App(levk::NotNull<levk::VfsFiles const*> vfs);

	void run(vk::SampleCountFlagBits samples);

  private:
	struct LoadInfo {
		std::string_view uri;
		levk::Transform transform;
		levk::ImportIndex import_index;
		levk::ImportIndex parent{levk::ImportIndex::eNone};
	};

	void on_key_up(int key, int mods) final;

	void on_file_drop(std::span<char const*> paths) final;

	void tick();
	virtual void tick(levk::Seconds dt);
	void render();
	virtual void render(levk::RenderContext& context) const;

	auto spawn_cameras() -> levk::EntityId;

	void on_loaded();

	void handle_file_drop(levk::CString path);
	void handle_asset_drop(std::string_view uri);
	void handle_gltf_drop(fs::path const& path);

	template <typename AssetT>
	auto try_load_mesh(std::string_view uri) -> levk::Ptr<typename AssetT::MeshType>;

	void load_scene(std::string_view uri);
	void load_node(levk::ImportedScene const& scene, levk::ImportIndex index, levk::ImportIndex parent);
	auto load_static_mesh(std::string_view uri) -> bool;
	auto load_skinned_mesh(std::string_view uri) -> bool;
	auto load_skybox(std::string_view uri) -> bool;
	auto attach_static_mesh(levk::Entity& out, std::string_view uri) -> bool;
	auto attach_skinned_mesh(levk::Entity& out, std::string_view uri) -> bool;

	virtual void inspect(levk::Seconds dt);

	void draw_general_window(levk::Seconds dt);
	virtual void draw_general_tabs(levk::Seconds dt);
	void draw_scene_tab();
	void draw_assets_tab(levk::Seconds dt);

	void draw_inspector_window();

	void draw_debug_window();

	levk::Logger m_log{"App"};

	levk::NotNull<levk::VfsFiles const*> m_vfs;
	std::unique_ptr<levk::IEngine> m_engine{};
	std::unique_ptr<levk::IAssetStore> m_asset_store{};
	std::unique_ptr<levk::IRenderCamera> m_render_camera{};
	std::unique_ptr<levk::IRenderCamera> m_shadow_camera{};

	levk::Scene m_scene{};
	levk::EntityId m_camera_id{};
	struct {
		levk::EntityId free_cam{};
		levk::EntityId orbit_cam{};
	} m_cameras{};
	levk::Degrees m_fov{levk::RenderContext::fov_v};
	glm::vec2 m_z_plane{levk::RenderContext::z_plane_v};

	int m_executor_workers{4};
	int m_executor_max_workers{};
	bool m_force_import{};

	Loader m_loader{};
	AssetTree m_asset_tree{};

	levk::DeltaTime m_delta_time{};
	mutable levk::RenderStats m_render_stats{};

	levk::TreeNodeId m_inspect_target{levk::TreeNodeId::eNone};
};
