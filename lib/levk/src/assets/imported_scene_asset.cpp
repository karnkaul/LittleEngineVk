#include <levk/asset_store.hpp>
#include <levk/assets/imported_scene_asset.hpp>
#include <levk/io/json_io.hpp>

namespace levk {
auto ImportedScene::get_node(ImportIndex const index) const -> ImportedNode const& {
	static auto const blank_v = ImportedNode{};
	auto const func = [index](ImportedNode const& n) { return n.index == index; };
	if (auto const it = std::ranges::find_if(nodes, func); it != nodes.end()) { return *it; }
	return blank_v;
}

auto ImportedSceneAsset::to_node(dj::Json const& json) -> ImportedNode {
	if (IAssetStore::read_asset_type_name(json) != ImportedNode::type_name_v) { return {}; }
	auto ret = ImportedNode{
		.index = static_cast<ImportIndex>(json["import_index"].as<std::int64_t>()),
		.name = json["name"].as<std::string>(),
		.mesh = json["mesh"].as<std::string>(),
		.skeleton = json["skeleton"].as<std::string>(),
	};
	if (auto const& parent = json["parent"]) { ret.parent = static_cast<ImportIndex>(parent.as<std::int64_t>()); }
	auto transform = Transform::Data{};
	from_json(json["transform"], transform);
	ret.transform.set_data(transform);
	if (auto const& camera = json["camera"]) {
		ret.camera.emplace();
		ret.camera->type = camera["type"].as_string() == "orthographic" ? ImportedCamera::Type::eOrthographic : ImportedCamera::Type::ePerspective;
		ret.camera->z_near = camera["z_near"].as<float>();
		if (auto const& z_far = camera["z_far"]) { ret.camera->z_far = z_far.as<float>(); }
		if (auto const& y_fov = camera["y_fov"]) { ret.camera->y_fov = Radians{y_fov.as<float>()}; }
	}
	for (auto const& child_index : json["children"].array_view()) { ret.children.push_back(ImportIndex{child_index.as<std::int64_t>()}); }
	return ret;
}

auto ImportedSceneAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	imported_scene.nodes.clear();
	imported_scene.root_nodes.clear();
	for (auto const& node : json["nodes"].array_view()) { imported_scene.nodes.push_back(to_node(node)); }
	for (auto const& index : json["root_nodes"].array_view()) { imported_scene.root_nodes.push_back(ImportIndex{index.as<std::int64_t>()}); }
	imported_scene.skybox = json["skybox"].as_string();

	return true;
}
} // namespace levk
