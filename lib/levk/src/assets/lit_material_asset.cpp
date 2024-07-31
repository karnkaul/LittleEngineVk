#include <djson/json.hpp>
#include <levk/asset_store.hpp>
#include <levk/assets/lit_material_asset.hpp>
#include <levk/assets/shader_asset.hpp>
#include <levk/assets/texture_asset.hpp>

namespace levk {
auto LitMaterialAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v || json["material_type"].as_string() != material::Lit::type_name_v) { return false; }

	auto const shader_info = LoadInfo{
		.uri = json["fragment_shader"].as_string(),
		.reload = info.reload,
	};
	auto const* frag = store.load<ShaderAsset>(shader_info);
	if (frag == nullptr) { return false; }

	material.emplace(store.get_engine().get_render_device(), frag->get_render_shader());
	material->fragment_shader = frag->get_render_shader();

	auto const get_texture = [&](std::string_view const key) -> Ptr<ITexture const> {
		if (auto const& uri = json[key]) {
			auto const texture_info = LoadInfo{
				.uri = uri.as_string(),
				.reload = info.reload,
			};
			if (auto const* asset = store.load<TextureAsset>(texture_info)) { return asset->texture.get(); }
		}
		return nullptr;
	};

	material->name = json["name"].as_string();

	if (auto const* texure = get_texture("base_colour")) { material->base_colour = texure; }
	if (auto const* texure = get_texture("metallic_roughness")) { material->metallic_roughness = texure; }
	if (auto const* texture = get_texture("emissive")) { material->emissive = texture; }
	material->normal = get_texture("normal");

	auto data = material::Lit::Data{};

	data.albedo = colour::to_u8(json["albedo"].as_string());
	data.emissive_factor = colour::to_u8(json["emissive_factor"].as_string());
	from_json(json["metallic"], data.metallic);
	from_json(json["roughness"], data.roughness);
	from_json(json["alpha_cutoff"], data.alpha_cutoff);
	from_json(json["is_transparent"], data.is_transparent);
	material->set_data(data);

	return true;
}
} // namespace levk
