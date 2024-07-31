#pragma once
#include <levk/asset_store.hpp>
#include <levk/executor.hpp>
#include <unordered_set>

namespace levk {
namespace load_stage {
inline constexpr auto shader_v = ExecStage::eDefault;
inline constexpr auto image_v = ExecStage::eDefault;
inline constexpr auto skeletal_animation_v = ExecStage::eDefault;
inline constexpr auto skeleton_v = dependent_exec_stage(skeletal_animation_v);
inline constexpr auto texture_v = dependent_exec_stage(image_v);
inline constexpr auto material_v = dependent_exec_stage(texture_v);
inline constexpr auto primitive_v = dependent_exec_stage(material_v);
inline constexpr auto mesh_v = dependent_exec_stage(primitive_v, skeleton_v);
inline constexpr auto imported_scene_v = dependent_exec_stage(mesh_v);
} // namespace load_stage

class AssetLoadList {
  public:
	explicit AssetLoadList(NotNull<IAssetStore*> store);

	void add_shader(std::string uri);
	void add_image(std::string uri);
	void add_skeletal_animation(std::string uri);

	void add_skeleton(std::string uri);
	void add_texture(std::string uri);

	void add_lit_material(std::string uri);

	void add_primitive(std::string uri);

	void add_mesh(std::string uri);

	void add_imported_scene(std::string uri);

	auto add_asset(std::string json_uri, Ptr<std::string> out_type_name = {}) -> bool;

	template <typename AssetT>
	void add_loader(ExecStage const stage, std::string uri) {
		if (m_added.contains(uri)) { return; }
		auto func = [store = m_store, uri = uri] { store->load<AssetT>(uri); };
		tasks.push_back(ExecTask{.stage = stage, .func = std::move(func)});
		m_added.insert(std::move(uri));
	}

	[[nodiscard]] auto execute_loads(std::optional<int> workers = {}) -> std::unique_ptr<IExecutor>;

	std::vector<ExecTask> tasks{};

  private:
	NotNull<IAssetStore*> m_store;

	std::unordered_set<std::string> m_added{};
};

void append_texture_load(NotNull<IAssetStore*> store, std::vector<ExecTask>& tasks);
} // namespace levk
