#pragma once
#include <unordered_map>
#include <engine/assets/asset_loader.hpp>
#include <engine/render/model.hpp>
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
	std::optional<graphics::Pipeline::CreateInfo> info;
	graphics::PFlags flags;
	std::string name;
	Ref<graphics::RenderContext> context;
	Hash shaderID;

	AssetLoadData(graphics::RenderContext& context) : context(context) {
	}
};

template <>
struct AssetLoader<graphics::Pipeline> {
	std::optional<graphics::Pipeline> load(AssetLoadInfo<graphics::Pipeline> const& info) const;
	bool reload(graphics::Pipeline& out_shader, AssetLoadInfo<graphics::Pipeline> const& info) const;
};

template <>
struct AssetLoadData<graphics::Texture> {
	kt::fixed_vector<io::Path, 6> imageIDs;
	graphics::Texture::Raw raw;
	io::Path prefix;
	std::string ext;
	std::string name;
	Ref<graphics::VRAM> vram;
	Hash samplerID;

	AssetLoadData(graphics::VRAM& vram) : vram(vram) {
	}
};

template <>
struct AssetLoader<graphics::Texture> {
	using Data = graphics::Texture::CreateInfo::Data;

	std::optional<graphics::Texture> load(AssetLoadInfo<graphics::Texture> const& info) const;
	bool reload(graphics::Texture& out_texture, AssetLoadInfo<graphics::Texture> const& info) const;

	std::optional<Data> data(AssetLoadInfo<graphics::Texture> const& info) const;
};

template <>
struct AssetLoadData<Model> {
	std::string modelID;
	io::Path jsonID;
	vk::Format texFormat = graphics::Texture::srgbFormat;
	Ref<graphics::VRAM> vram;
	Ref<graphics::Sampler const> sampler;

	AssetLoadData(graphics::VRAM& vram, graphics::Sampler const& sampler) : vram(vram), sampler(sampler) {
	}
};

template <>
struct AssetLoader<Model> {
	std::optional<Model> load(AssetLoadInfo<Model> const& info) const;
	bool reload(Model& out_texture, AssetLoadInfo<Model> const& info) const;
};
} // namespace le
