#pragma once
#include <unordered_map>
#include <engine/assets/asset_loader.hpp>
#include <engine/render/bitmap_font.hpp>
#include <engine/render/model.hpp>
#include <graphics/render/context.hpp>
#include <graphics/shader.hpp>
#include <graphics/texture.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>

namespace le {
template <>
struct AssetLoadData<graphics::Shader> {
	std::string name;
	std::unordered_map<graphics::Shader::Type, io::Path> shaderPaths;
	not_null<graphics::Device*> device;

	AssetLoadData(not_null<graphics::Device*> device) : device(device) {}
};

template <>
struct AssetLoader<graphics::Shader> {
	using Data = graphics::Shader::SpirVMap;

	std::optional<graphics::Shader> load(AssetLoadInfo<graphics::Shader> const& info) const;
	bool reload(graphics::Shader& out_shader, AssetLoadInfo<graphics::Shader> const& info) const;

	std::optional<Data> data(AssetLoadInfo<graphics::Shader> const& info) const;
};

template <>
struct AssetLoadData<graphics::Pipeline> {
	std::optional<graphics::Pipeline::CreateInfo> info;
	graphics::PFlags flags;
	std::string name;
	not_null<graphics::RenderContext*> context;
	Hash shaderID;
	bool gui = false;

	AssetLoadData(not_null<graphics::RenderContext*> context) : context(context) {}
};

template <>
struct AssetLoader<graphics::Pipeline> {
	std::optional<graphics::Pipeline> load(AssetLoadInfo<graphics::Pipeline> const& info) const;
	bool reload(graphics::Pipeline& out_shader, AssetLoadInfo<graphics::Pipeline> const& info) const;
};

template <>
struct AssetLoadData<graphics::Texture> {
	kt::fixed_vector<io::Path, 6> imageIDs;
	graphics::Bitmap bitmap;
	graphics::Cubemap cubemap;
	io::Path prefix;
	std::string ext;
	std::optional<vk::Format> forceFormat;
	not_null<graphics::VRAM*> vram;
	Hash samplerID;
	graphics::Texture::Payload payload = graphics::Texture::Payload::eColour;

	AssetLoadData(not_null<graphics::VRAM*> vram) : vram(vram) {}
};

template <>
struct AssetLoader<graphics::Texture> {
	using Data = graphics::Texture::CreateInfo::Data;

	std::optional<graphics::Texture> load(AssetLoadInfo<graphics::Texture> const& info) const;
	bool reload(graphics::Texture& out_texture, AssetLoadInfo<graphics::Texture> const& info) const;

	std::optional<Data> data(AssetLoadInfo<graphics::Texture> const& info) const;
};

template <>
struct AssetLoadData<BitmapFont> {
	io::Path jsonID;
	std::optional<vk::Format> forceFormat;
	not_null<graphics::VRAM*> vram;
	Hash samplerID;

	AssetLoadData(not_null<graphics::VRAM*> vram) : vram(vram) {}
};

template <>
struct AssetLoader<BitmapFont> {
	std::optional<BitmapFont> load(AssetLoadInfo<BitmapFont> const& info) const;
	bool reload(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const;

	bool load(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const;
};

template <>
struct AssetLoadData<Model> {
	std::string modelID;
	io::Path jsonID;
	std::optional<vk::Format> forceFormat;
	not_null<graphics::VRAM*> vram;
	Hash samplerID;

	AssetLoadData(not_null<graphics::VRAM*> vram) : vram(vram) {}
};

template <>
struct AssetLoader<Model> {
	std::optional<Model> load(AssetLoadInfo<Model> const& info) const;
	bool reload(Model& out_model, AssetLoadInfo<Model> const& info) const;
};
} // namespace le
