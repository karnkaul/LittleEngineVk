#pragma once
#include <engine/assets/asset_loader.hpp>
#include <engine/render/bitmap_font.hpp>
#include <engine/render/draw_group.hpp>
#include <engine/render/model.hpp>
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
struct AssetLoadData<PipelineState> {
	graphics::ShaderSpec shader;
	vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
	vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
	graphics::PFlags flags;
	f32 lineWidth = 1.0f;
};

template <>
struct AssetLoader<PipelineState> {
	std::unique_ptr<PipelineState> load(AssetLoadInfo<PipelineState> const& info) const;
	bool reload(PipelineState& out_ps, AssetLoadInfo<PipelineState> const& info) const;

	static PipelineState from(AssetLoadData<PipelineState> const& data);
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
	using Data = std::variant<graphics::Bitmap, graphics::BmpBytes, graphics::Cubemap, graphics::CubeBytes>;

	std::unique_ptr<graphics::Texture> load(AssetLoadInfo<graphics::Texture> const& info) const;
	bool reload(graphics::Texture& out_texture, AssetLoadInfo<graphics::Texture> const& info) const;

	bool load(graphics::Texture& out_texture, Data const& data, vk::Sampler sampler, std::optional<vk::Format> format) const;
	std::optional<Data> data(AssetLoadInfo<graphics::Texture> const& info) const;
};

template <>
struct AssetLoadData<BitmapFont> {
	io::Path jsonURI;
	std::optional<vk::Format> forceFormat;
	not_null<graphics::VRAM*> vram;
	Hash samplerURI;

	AssetLoadData(not_null<graphics::VRAM*> vram) : vram(vram) {}
};

template <>
struct AssetLoader<BitmapFont> {
	std::unique_ptr<BitmapFont> load(AssetLoadInfo<BitmapFont> const& info) const;
	bool reload(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const;

	bool load(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const;
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
