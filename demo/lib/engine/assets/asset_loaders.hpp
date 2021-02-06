#pragma once
#include <unordered_map>
#include <engine/assets/asset_loader.hpp>
#include <graphics/render_context.hpp>
#include <graphics/shader.hpp>
#include <graphics/texture.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>

namespace le {
template <>
struct AssetLoadData<graphics::Shader> {
	Ref<graphics::Device> device;
	std::unordered_map<graphics::Shader::Type, io::Path> shaderPaths;
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
	Ref<graphics::RenderContext> context;
	std::optional<graphics::Pipeline::CreateInfo> info;
	graphics::PFlags flags;
	std::string name;
	Hash shaderID;
	mutable AssetLoadInfo<graphics::Shader>::OnModified::Tk onShaderReload;
};

template <>
struct AssetLoader<graphics::Pipeline> {
	std::optional<graphics::Pipeline> load(AssetLoadInfo<graphics::Pipeline> const& info) const;
	bool reload(graphics::Pipeline& out_shader, AssetLoadInfo<graphics::Pipeline> const& info) const;
};

template <>
struct AssetLoadData<graphics::Texture> {
	Ref<graphics::VRAM> vram;
	kt::fixed_vector<io::Path, 6> imageIDs;
	graphics::Texture::Raw raw;
	io::Path prefix;
	std::string ext;
	std::string name;
	Hash samplerID;
};

template <>
struct AssetLoader<graphics::Texture> {
	using Data = graphics::Texture::CreateInfo::Data;

	std::optional<graphics::Texture> load(AssetLoadInfo<graphics::Texture> const& info) const;
	bool reload(graphics::Texture& out_texture, AssetLoadInfo<graphics::Texture> const& info) const;

	std::optional<Data> data(AssetLoadInfo<graphics::Texture> const& info) const;
};
} // namespace le
