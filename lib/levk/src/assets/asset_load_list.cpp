#include <djson/json.hpp>
#include <levk/assets/asset_load_list.hpp>
#include <levk/assets/image_asset.hpp>
#include <levk/assets/lit_material_asset.hpp>
#include <levk/assets/mesh_asset.hpp>
#include <levk/assets/primitive_asset.hpp>
#include <levk/assets/scene_info_asset.hpp>
#include <levk/assets/shader_asset.hpp>
#include <levk/assets/skeletal_animation_asset.hpp>
#include <levk/assets/skeleton_asset.hpp>
#include <levk/assets/texture_asset.hpp>

namespace levk {
AssetLoadList::AssetLoadList(NotNull<IAssetStore*> store) : m_store(store) {}

void AssetLoadList::add_shader(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }
	add_loader<ShaderAsset>(load_stage::shader_v, std::move(uri));
}

void AssetLoadList::add_image(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }
	add_loader<ImageAsset>(load_stage::image_v, std::move(uri));
}

void AssetLoadList::add_skeletal_animation(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }
	add_loader<SkeletalAnimationAsset>(load_stage::skeletal_animation_v, std::move(uri));
}

void AssetLoadList::add_skeleton(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }

	auto const json = m_store->get_vfs().load_json(uri);
	if (!json) { return; }

	for (auto const& animation : json["animations"].array_view()) { add_skeletal_animation(animation.as<std::string>()); }

	add_loader<SkeletonAsset>(load_stage::skeleton_v, std::move(uri));
}

void AssetLoadList::add_texture(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }

	auto const json = m_store->get_vfs().load_json(uri);
	if (!json) { return; }

	add_image(json["image"].as<std::string>());

	add_loader<TextureAsset>(load_stage::texture_v, std::move(uri));
}

void AssetLoadList::add_cubemap(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }

	auto const json = m_store->get_vfs().load_json(uri);
	if (!json) { return; }

	for (auto const& image : json["layers"].array_view()) { add_image(image.as<std::string>()); }

	add_loader<CubemapAsset>(load_stage::cubemap_v, std::move(uri));
}

void AssetLoadList::add_lit_material(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }

	auto const json = m_store->get_vfs().load_json(uri);
	if (!json) { return; }

	add_shader(json["fragment_shader"].as<std::string>());

	add_texture(json["base_colour"].as<std::string>());
	add_texture(json["metallic_roughness"].as<std::string>());
	add_texture(json["emissive"].as<std::string>());

	add_loader<LitMaterialAsset>(load_stage::material_v, std::move(uri));
}

void AssetLoadList::add_primitive(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }

	auto const json = m_store->get_vfs().load_json(uri);
	if (!json) { return; }

	add_shader(json["vertex_shader"].as<std::string>());

	auto const material_uri = json["material"].as_string();
	auto const material_json = m_store->get_vfs().load_json(material_uri);
	if (material_json["material_type"].as_string() == material::Lit::type_name_v) { add_lit_material(std::string{material_uri}); }

	if (json["type_name"].as_string() == SkinnedPrimitiveAsset::type_name_v) {
		add_loader<SkinnedPrimitiveAsset>(load_stage::primitive_v, std::move(uri));
	} else {
		add_loader<StaticPrimitiveAsset>(load_stage::primitive_v, std::move(uri));
	}
}

void AssetLoadList::add_mesh(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }

	auto const json = m_store->get_vfs().load_json(uri);
	if (!json) { return; }

	add_skeleton(json["skeleton"].as<std::string>());

	for (auto const& primitive : json["primitives"].array_view()) { add_primitive(primitive.as<std::string>()); }

	if (json["type_name"].as_string() == SkinnedMeshAsset::type_name_v) {
		add_loader<SkinnedMeshAsset>(load_stage::mesh_v, std::move(uri));
	} else {
		add_loader<StaticMeshAsset>(load_stage::mesh_v, std::move(uri));
	}
}

void AssetLoadList::add_imported_scene(std::string uri) {
	if (uri.empty() || m_added.contains(uri)) { return; }

	auto const json = m_store->get_vfs().load_json(uri);
	if (!json) { return; }

	add_cubemap(json["skybox"].as<std::string>());
	for (auto const& node : json["nodes"].array_view()) { add_mesh(node["mesh"].as<std::string>()); }

	add_loader<SceneInfoAsset>(load_stage::imported_scene_v, std::move(uri));
}

auto AssetLoadList::add_asset(std::string uri, Ptr<std::string> out_type_name) -> bool {
	auto type_name = m_store->get_asset_type_name(std::string_view{uri});
	if (out_type_name != nullptr) {
		*out_type_name = std::move(type_name);
	} else {
		out_type_name = &type_name;
	}

	auto ret = false;
	if (ret = *out_type_name == TextureAsset::type_name_v; ret) {
		add_texture(std::move(uri));
	} else if (ret = *out_type_name == SkeletonAsset::type_name_v; ret) {
		add_skeleton(std::move(uri));
	} else if (ret = *out_type_name == LitMaterialAsset::type_name_v; ret) {
		add_lit_material(std::move(uri));
	} else if (ret = *out_type_name == StaticMeshAsset::type_name_v || *out_type_name == SkinnedMeshAsset::type_name_v; ret) {
		add_mesh(std::move(uri));
	} else if (ret = *out_type_name == SceneInfoAsset::type_name_v; ret) {
		add_imported_scene(uri);
	}

	return ret;
}

auto AssetLoadList::execute_loads(std::optional<int> const workers) -> std::unique_ptr<IExecutor> {
	if (tasks.empty()) { return {}; }
	return create_executor(std::move(tasks), workers);
}
} // namespace levk
