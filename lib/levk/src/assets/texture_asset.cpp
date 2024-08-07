#include <djson/json.hpp>
#include <levk/asset_store.hpp>
#include <levk/assets/image_asset.hpp>
#include <levk/assets/texture_asset.hpp>
#include <levk/image_file.hpp>

namespace levk {
namespace {
constexpr auto get_filter(std::string_view const in) {
	if (in == "nearest") { return vk::Filter::eNearest; }
	return vk::Filter::eLinear;
}

constexpr auto get_address_mode(std::string_view const in) {
	if (in == "clamp_edge") { return vk::SamplerAddressMode::eClampToEdge; }
	if (in == "clamp_border") { return vk::SamplerAddressMode::eClampToBorder; }
	if (in == "mirror") { return vk::SamplerAddressMode::eMirroredRepeat; }
	return vk::SamplerAddressMode::eRepeat;
}

[[nodiscard]] auto get_flags(dj::Json const& json) {
	auto ret = TextureFlags{};
	if (auto const& mip_map = json["mip_map"]) {
		if (!mip_map.as_bool(dj::true_v)) { ret.set(TextureFlag::eNoMipMaps); }
	}
	if (auto const& linear = json["linear"]) {
		if (linear.as_bool(dj::false_v)) { ret.set(TextureFlag::eLinear); }
	}
	return ret;
}

[[nodiscard]] auto get_sampler(dj::Json const& json) {
	auto ret = TextureSampler{};
	if (!json) { return ret; }
	ret.mag_filter = get_filter(json["mag_filter"].as_string());
	ret.min_filter = get_filter(json["min_filter"].as_string());
	ret.wrap_u = get_address_mode(json["wrap_u"].as_string());
	ret.wrap_v = get_address_mode(json["wrap_v"].as_string());
	return ret;
}
} // namespace

auto TextureAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	auto const& image = json["image"];
	if (!image) { return false; }

	auto const image_info = LoadInfo{
		.uri = image.as_string(),
		.reload = info.reload,
	};
	auto const* image_asset = store.load<ImageAsset>(image_info);
	if (image_asset == nullptr) { return false; }

	auto const flags = get_flags(json);

	texture = store.get_engine().get_render_device().create_texture(image_asset->image.get_bitmap_view(), flags);
	texture->sampler = get_sampler(json["sampler"]);
	texture->name = json["name"].as_string();

	return true;
}

auto CubemapAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	auto const& in_layers = json["layers"];
	if (in_layers.array_view().size() != 6) { return false; }

	auto images = std::array<Ptr<ImageAsset const>, 6>{};
	auto extent = std::optional<glm::ivec2>{};
	for (auto [layer, image] : std::ranges::zip_view(in_layers.array_view(), images)) {
		auto const image_info = LoadInfo{
			.uri = layer.as_string(),
			.reload = info.reload,
		};
		image = store.load<ImageAsset>(image_info);
		if (image == nullptr || image->image.is_empty()) { return false; }

		if (!extent) {
			extent = image->image.get_bitmap_view().extent;
			continue;
		}

		// all image layers must have the same extent.
		if (image->image.get_bitmap_view().extent != *extent) { return false; }
	}

	auto out_layers = CubemapLayers{};
	for (auto [image, layer] : std::ranges::zip_view(images, out_layers)) { layer = image->image.get_bitmap_view(); }

	auto const flags = get_flags(json);

	cubemap = store.get_engine().get_render_device().create_cubemap(out_layers, flags);
	cubemap->sampler = get_sampler(json["sampler"]);
	cubemap->name = json["name"].as_string();

	return true;
}
} // namespace levk
