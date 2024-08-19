#include <levk/asset_store.hpp>
#include <levk/assets/scene_info_asset.hpp>
#include <levk/io/tree_io.hpp>

namespace levk {
auto SceneInfoAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	scene = {};

	struct PerNode {
		void operator()(dj::Json const& in_node, ImportedNode& out_node) const {
			out_node.mesh = in_node["mesh"].as<std::string>();
			out_node.skeleton = in_node["skeleton"].as<std::string>();
			if (auto const& camera = in_node["camera"]) {
				out_node.camera.emplace();
				out_node.camera->type = camera["type"].as_string() == "orthographic" ? ImportedCamera::Type::eOrthographic : ImportedCamera::Type::ePerspective;
				out_node.camera->z_near = camera["z_near"].as<float>();
				if (auto const& z_far = camera["z_far"]) { out_node.camera->z_far = z_far.as<float>(); }
				if (auto const& y_fov = camera["y_fov"]) { out_node.camera->y_fov = Radians{y_fov.as<float>()}; }
			}
		}
	};

	scene = JsonToTree<ImportedNode, SceneInfo>{}.import_tree(json["nodes"], PerNode{});

	scene.skybox = json["skybox"].as_string();
	if (auto const& main_light = json["main_light"]) { from_json(main_light, scene.main_light.emplace()); }

	return true;
}
} // namespace levk
