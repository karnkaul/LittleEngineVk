#pragma once
#include <engine/resources/resource_types.hpp>
#include <gfx/common.hpp>
#include <core/delegate.hpp>
#include <resources/monitor.hpp>

namespace le::res
{
template <typename T, typename TImpl>
struct TResource final
{
	static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");

	typename T::Info info;
	T resource;
	TImpl impl;
};

struct ImplBase
{
	stdfs::path id;
	GUID guid;
	Status status = Status::eIdle;
	bool bLoadedOnce = false;
};

struct ILoadable
{
	virtual bool update() = 0;
};

struct IReloadable
{
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	Delegate<> onReload;

	virtual bool checkReload() = 0;
#endif
};

struct Shader::Impl : ImplBase, IReloadable
{
	static constexpr std::array<vk::ShaderStageFlagBits, std::size_t(Shader::Type::eCOUNT_)> s_typeToFlagBit = {vk::ShaderStageFlagBits::eVertex,
																												vk::ShaderStageFlagBits::eFragment};

	static std::string_view s_spvExt;
	static std::string_view s_vertExt;
	static std::string_view s_fragExt;

	EnumArray<bytearray, Shader::Type> codeMap;
	std::array<vk::ShaderModule, std::size_t(Shader::Type::eCOUNT_)> shaders;
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	Monitor monitor;
#endif

	static std::string extension(stdfs::path const& id);

	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();

	vk::ShaderModule module(Shader::Type type) const;

	std::map<Shader::Type, vk::ShaderModule> modules() const;

#if defined(LEVK_SHADER_COMPILER)
	bool glslToSpirV(stdfs::path const& id, bytearray& out_bytes);
	bool loadGlsl(stdfs::path const& id, Shader::Type type);
#endif
	void loadAllSpirV();

#if defined(LEVK_RESOURCES_HOT_RELOAD)
	bool checkReload() override;
#endif
};

struct Sampler::Impl : ImplBase
{
	vk::Sampler sampler;

	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();
};

struct Texture::Impl : ImplBase, ILoadable, IReloadable
{
	gfx::Image active;
	std::vector<Texture::Raw> raws;
	vk::ImageView imageView;
	vk::ImageViewType type;
	vk::Format colourSpace;
	std::future<void> copied;
	bool bStbiRaw = false;

#if defined(LEVK_RESOURCES_HOT_RELOAD)
	Monitor monitor;
	gfx::Image standby;
	std::vector<stdfs::path> imgIDs;
#endif

	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();

	bool update() override;
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	bool checkReload() override;
#endif
};

struct Material::Impl : ImplBase
{
	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();
};

struct Mesh::Impl : ImplBase, ILoadable
{
	struct Data
	{
		gfx::Buffer buffer;
		std::future<void> copied;
		u32 count = 0;
		void* pMem = nullptr;
	};

	Data vbo;
	Data ibo;

	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();

	bool update() override;

	void updateGeometry(Info& out_info, gfx::Geometry geometry);
};

struct Font::Impl : ImplBase, ILoadable
{
	std::array<Glyph, maxVal<u8>()> glyphs;
	res::Material::Inst material;
	Glyph blankGlyph;
	res::Texture sheet;

	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();

	bool update() override;

	gfx::Geometry generate(Text const& text) const;
};

Shader::Impl* impl(Shader shader);
Sampler::Impl* impl(Sampler sampler);
Texture::Impl* impl(Texture texture);
Material::Impl* impl(Material material);
Mesh::Impl* impl(Mesh mesh);
Font::Impl* impl(Font font);

Shader::Info* infoRW(Shader shader);
Sampler::Info* infoRW(Sampler sampler);
Texture::Info* infoRW(Texture texture);
Material::Info* infoRW(Material material);
Mesh::Info* infoRW(Mesh mesh);
Font::Info* infoRW(Font font);

#if defined(LEVK_EDITOR)
std::vector<Shader> loadedShaders();
std::vector<Sampler> loadedSamplers();
std::vector<Texture> loadedTextures();
std::vector<Material> loadedMaterials();
std::vector<Mesh> loadedMeshes();
std::vector<Font> loadedFonts();
#endif

bool isLoading(GUID guid);

void init();
void update();
void waitIdle();
void deinit();

#if defined(LEVK_EDITOR)
template <typename T>
std::vector<T> loaded()
{
	if constexpr (std::is_same_v<T, Shader>)
	{
		return loadedShaders();
	}
	else if constexpr (std::is_same_v<T, Sampler>)
	{
		return loadedSamplers();
	}
	else if constexpr (std::is_same_v<T, Texture>)
	{
		return loadedTextures();
	}
	else if constexpr (std::is_same_v<T, Material>)
	{
		return loadedMaterials();
	}
	else if constexpr (std::is_same_v<T, Mesh>)
	{
		return loadedMeshes();
	}
	else if constexpr (std::is_same_v<T, Font>)
	{
		return loadedFonts();
	}
	else
	{
		static_assert(alwaysFalse<T>, "Invalid Type!");
	}
}
#endif

struct Service final
{
	Service();
	~Service();
};
} // namespace le::res