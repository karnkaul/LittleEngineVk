#pragma once
#include <ktl/fixed_vector.hpp>
#include <ktl/kvariant.hpp>
#include <levk/engine/assets/asset_loader.hpp>
#include <levk/engine/render/layer.hpp>
#include <levk/engine/render/model.hpp>
#include <levk/graphics/font/font.hpp>
#include <levk/graphics/render/context.hpp>
#include <levk/graphics/texture.hpp>
#include <unordered_map>

namespace le {
template <>
struct AssetLoadData<graphics::SpirV> {
	io::Path uri;
	graphics::ShaderType type = graphics::ShaderType::eFragment;
};

template <>
struct AssetLoader<graphics::SpirV> {
	AssetStorage<graphics::SpirV> load(AssetLoadInfo<graphics::SpirV> const& info) const;
	bool reload(graphics::SpirV& out_code, AssetLoadInfo<graphics::SpirV> const& info) const;

	bool load(graphics::SpirV& out_code, AssetLoadInfo<graphics::SpirV> const& info) const;
};

template <>
struct AssetLoadData<graphics::Texture> {
	ktl::fixed_vector<io::Path, 6> imageURIs;
	Bitmap bitmap;
	Cubemap cubemap;
	io::Path prefix;
	std::string ext;
	std::optional<vk::Format> forceFormat;
	not_null<graphics::VRAM*> vram;
	Hash samplerURI;
	graphics::Texture::Payload payload = graphics::Texture::Payload::eColour;

	AssetLoadData(not_null<graphics::VRAM*> vram) : vram(vram) {}
};

template <>
struct AssetLoader<graphics::Texture> {
	using Cube = std::array<ImageData, 6>;
	using Data = ktl::kvariant<Bitmap, ImageData, Cubemap, Cube>;

	AssetStorage<graphics::Texture> load(AssetLoadInfo<graphics::Texture> const& info) const;
	bool reload(graphics::Texture& out_texture, AssetLoadInfo<graphics::Texture> const& info) const;

	bool load(graphics::Texture& out_texture, Data const& data, vk::Sampler sampler, std::optional<vk::Format> format) const;
	std::optional<Data> data(AssetLoadInfo<graphics::Texture> const& info) const;
};

template <>
struct AssetLoadData<graphics::Font> {
	graphics::Font::Info info;
	io::Path ttfURI;
	not_null<graphics::VRAM*> vram;

	AssetLoadData(not_null<graphics::VRAM*> vram) : vram(vram) {}
};

template <>
struct AssetLoader<graphics::Font> {
	AssetStorage<graphics::Font> load(AssetLoadInfo<graphics::Font> const& info) const;
	bool reload(graphics::Font& out_font, AssetLoadInfo<graphics::Font> const& info) const;
};

template <>
struct AssetLoadData<Model> {
	std::string modelURI;
	io::Path jsonURI;
	std::optional<vk::Format> forceFormat;
	not_null<graphics::VRAM*> vram;
	Hash samplerURI;

	AssetLoadData(not_null<graphics::VRAM*> vram) : vram(vram) {}
};

template <>
struct AssetLoader<Model> {
	AssetStorage<Model> load(AssetLoadInfo<Model> const& info) const;
	bool reload(Model& out_model, AssetLoadInfo<Model> const& info) const;
};
} // namespace le
