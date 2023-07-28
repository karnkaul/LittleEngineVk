#include <spaced/core/logger.hpp>
#include <spaced/resources/bin_data.hpp>
#include <spaced/resources/material_asset.hpp>
#include <spaced/resources/resources.hpp>
#include <spaced/resources/texture_asset.hpp>
#include <spaced/vfs/file_reader.hpp>

namespace spaced {
namespace {
template <glm::length_t Len, typename T>
auto to_glm_vec(glm::vec<Len, T>& out, dj::Json const& array) -> void {
	if (!array.is_array()) { return; }
	out[0] = array[0].as<T>();
	if constexpr (Len > 1) { out[1] = array[1].as<T>(out[1]); }
	if constexpr (Len > 2) { out[2] = array[2].as<T>(out[2]); }
	if constexpr (Len > 3) { out[3] = array[3].as<T>(out[3]); }
}

constexpr auto to_alpha_mode(std::string_view const in, graphics::AlphaMode const fallback) -> graphics::AlphaMode {
	if (in == "blend") { return graphics::AlphaMode::eBlend; }
	if (in == "mask") { return graphics::AlphaMode::eMask; }
	if (in == "opaque") { return graphics::AlphaMode::eOpaque; }
	return fallback;
}

auto const g_log{logger::Logger{MaterialAsset::type_name_v}};

[[nodiscard]] auto make_unlit(dj::Json const& json) -> std::unique_ptr<graphics::UnlitMaterial> {
	auto ret = std::make_unique<graphics::UnlitMaterial>();

	if (auto const& base_colour = json["base_colour"]) {
		if (auto* texture_asset = Resources::self().load<TextureAsset>(base_colour.as<std::string>())) { ret->texture = &texture_asset->texture; }
	}

	return ret;
}

template <std::derived_from<graphics::LitMaterial> T = graphics::LitMaterial>
[[nodiscard]] auto make_lit(dj::Json const& json) -> std::unique_ptr<T> {
	auto ret = std::make_unique<T>();

	if (auto const& base_colour = json["base_colour"]) {
		if (auto* texture_asset = Resources::self().load<TextureAsset>(base_colour.as<std::string>())) { ret->base_colour = &texture_asset->texture; }
	}
	if (auto const& metallic_roughness = json["metallic_roughness"]) {
		if (auto* texture_asset = Resources::self().load<TextureAsset>(metallic_roughness.as<std::string>())) {
			ret->metallic_roughness = &texture_asset->texture;
		}
	}
	if (auto const& emissive = json["emissive"]) {
		if (auto* texture_asset = Resources::self().load<TextureAsset>(emissive.as<std::string>())) { ret->emissive = &texture_asset->texture; }
	}

	if (auto const& in_albedo = json["albedo"]) {
		auto albedo = ret->albedo.to_vec4();
		to_glm_vec(albedo, in_albedo);
		ret->albedo = graphics::Rgba::from(albedo);
	}
	if (auto const& in_emissive_factor = json["emissive_factor"]) { to_glm_vec(ret->emissive_factor, in_emissive_factor); }

	ret->metallic = json["metallic"].as<float>(ret->metallic);
	ret->roughness = json["roughness"].as<float>(ret->roughness);
	ret->alpha_cutoff = json["alpha_cutoff"].as<float>(ret->alpha_cutoff);
	ret->alpha_mode = to_alpha_mode(json["alpha_mode"].as_string(), ret->alpha_mode);

	return ret;
}
} // namespace

auto MaterialAsset::try_load(Uri const& uri) -> bool {
	auto const json = read_json(uri);
	if (!json) { return false; }

	auto const material_type = json["material_type"].as_string();
	if (material_type == graphics::LitMaterial::material_type_v) {
		material = make_lit(json);
		return true;
	}

	if (material_type == graphics::SkinnedMaterial::material_type_v) {
		material = make_lit<graphics::SkinnedMaterial>(json);
		return true;
	}

	material = make_unlit(json);
	return true;
}
} // namespace spaced
