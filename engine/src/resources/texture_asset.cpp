#include <spaced/graphics/image_file.hpp>
#include <spaced/resources/texture_asset.hpp>

namespace spaced {
namespace {
constexpr auto to_colour_space(std::string_view const str) -> graphics::ColourSpace {
	return str == "linear" ? graphics::ColourSpace::eLinear : graphics::ColourSpace::eSrgb;
}

auto to_sampler(dj::Json const& json) -> graphics::TextureSampler {
	if (!json) { return {}; }

	static constexpr auto to_wrap = [](std::string_view const str) {
		if (str == "clamp_edge") { return graphics::TextureSampler::Wrap::eClampEdge; }
		return graphics::TextureSampler::Wrap::eRepeat;
	};
	static constexpr auto to_filter = [](std::string_view const str) {
		if (str == "nearest") { return graphics::TextureSampler::Filter::eNearest; }
		return graphics::TextureSampler::Filter::eLinear;
	};

	auto ret = graphics::TextureSampler{};
	ret.wrap_s = to_wrap(json["wrap_s"].as_string());
	ret.wrap_t = to_wrap(json["wrap_t"].as_string());
	ret.min = to_filter(json["min"].as_string());
	ret.mag = to_filter(json["mag"].as_string());
	return ret;
}
} // namespace

auto TextureAsset::try_load(dj::Json const& json) -> bool {
	if (!json) { return false; }

	auto const image_uri = json["image"].as<std::string>();
	if (image_uri.empty()) { return false; }

	auto const bytes = read_bytes(image_uri);
	auto const colour_space = to_colour_space(json["colour_space"].as_string());

	texture.sampler = to_sampler(json["sampler"]);
	return try_load(bytes, colour_space);
}

auto TextureAsset::try_load(std::span<std::uint8_t const> bytes, graphics::ColourSpace colour_space) -> bool {
	if (bytes.empty()) { return false; }

	auto image = graphics::ImageFile{};
	if (!image.decompress(bytes)) { return false; }

	if (texture.colour_space() != colour_space) { texture = graphics::Texture{colour_space}; }
	return texture.write(image.bitmap());
}

auto TextureAsset::try_load(Uri const& uri) -> bool {
	if (uri.extension() == ".json") { return try_load(read_json(uri)); }
	return try_load(read_bytes(uri), graphics::ColourSpace::eSrgb);
}
} // namespace spaced
