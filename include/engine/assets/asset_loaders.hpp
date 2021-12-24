#pragma once
#include <engine/assets/asset_loader.hpp>
#include <engine/render/layer.hpp>
#include <engine/render/model.hpp>
#include <graphics/font/font.hpp>
#include <graphics/render/context.hpp>
#include <graphics/texture.hpp>
#include <ktl/fixed_vector.hpp>
#include <unordered_map>
#include <variant>

namespace le {
template <>
struct AssetLoadData<graphics::SpirV> {
	io::Path uri;
	graphics::ShaderType type = graphics::ShaderType::eFragment;
};

template <>
struct AssetLoader<graphics::SpirV> {
	std::unique_ptr<graphics::SpirV> load(AssetLoadInfo<graphics::SpirV> const& info) const;
	bool reload(graphics::SpirV& out_code, AssetLoadInfo<graphics::SpirV> const& info) const;

	bool load(graphics::SpirV& out_code, AssetLoadInfo<graphics::SpirV> const& info) const;
};

template <>
struct AssetLoadData<graphics::Texture> {
	ktl::fixed_vector<io::Path, 6> imageURIs;
	graphics::Bitmap bitmap;
	graphics::Cubemap cubemap;
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
	using Cube = std::array<graphics::ImageData, 6>;
	using Data = std::variant<graphics::Bitmap, graphics::ImageData, graphics::Cubemap, Cube>;

	std::unique_ptr<graphics::Texture> load(AssetLoadInfo<graphics::Texture> const& info) const;
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
	std::unique_ptr<graphics::Font> load(AssetLoadInfo<graphics::Font> const& info) const;
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
	std::unique_ptr<Model> load(AssetLoadInfo<Model> const& info) const;
	bool reload(Model& out_model, AssetLoadInfo<Model> const& info) const;
};
} // namespace le
