#include <spaced/core/zip_ranges.hpp>
#include <spaced/graphics/image_file.hpp>
#include <spaced/resources/texture_asset.hpp>
#include <algorithm>
#include <future>

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

auto CubemapAsset::try_load(Uri const& uri) -> bool {
	auto const json_uri = [&uri] {
		auto ret = uri;
		if (ret.extension().empty()) { ret = std::string{uri} + "/cubemap.json"; }
		return ret;
	}();

	auto const json = read_json(json_uri);
	if (!json) { return false; }

	auto compressed_futures = std::array<std::future<std::vector<std::uint8_t>>, graphics::Image::cubemap_layers_v>{};
	for (auto [future, uri] : zip_ranges(compressed_futures, json["images"].array_view())) {
		future = std::async(std::launch::async, [this, uri = uri.as<std::string>()] { return read_bytes(uri); });
	}

	if (std::ranges::any_of(compressed_futures, [](auto const& future) { return !future.valid(); })) { return false; }

	auto image_file_futures = std::array<std::shared_future<graphics::ImageFile>, graphics::Image::cubemap_layers_v>{};
	for (auto [file, future] : zip_ranges(image_file_futures, compressed_futures)) {
		auto func = [future = std::move(future)]() mutable {
			auto ret = graphics::ImageFile{};
			ret.decompress(future.get());
			return ret;
		};
		file = std::async(std::launch::async, std::move(func)).share();
	}

	auto bitmaps = std::array<graphics::Bitmap, graphics::Image::cubemap_layers_v>{};
	for (auto [bitmap, file] : zip_ranges(bitmaps, image_file_futures)) {
		auto const& image_file = file.get();
		if (!image_file) { return false; }
		bitmap = image_file.bitmap();
	}

	auto const colour_space = to_colour_space(json["colour_space"].as_string());
	if (cubemap.colour_space() != colour_space) { cubemap = graphics::Cubemap{colour_space}; }

	return cubemap.write(bitmaps);
}
} // namespace spaced
