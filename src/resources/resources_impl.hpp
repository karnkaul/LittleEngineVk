#pragma once
#include <memory>
#include <engine/resources/resource_types.hpp>
#include <gfx/common.hpp>
#include <core/delegate.hpp>
#include <core/path_tree.hpp>
#include <resources/monitor.hpp>

namespace le::res
{
template <typename T, typename TImpl>
struct TResource final
{
	static_assert(std::is_base_of_v<Resource<T>, T>, "T must derive from Resource");

	typename T::Info info;
	T resource;
	std::unique_ptr<TImpl> uImpl;
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
	bool update()
	{
		return true;
	}
};

struct IReloadable
{
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	Delegate<> onReload;
	Monitor monitor;

	bool checkReload()
	{
		return true;
	}
#endif
};

struct Shader::Impl : ImplBase, IReloadable
{
	static constexpr std::array<vk::ShaderStageFlagBits, std::size_t(Shader::Type::eCOUNT_)> s_typeToFlagBit = {vk::ShaderStageFlagBits::eVertex,
																												vk::ShaderStageFlagBits::eFragment};

	inline static std::string_view s_spvExt = ".spv";
	inline static std::string_view s_vertExt = ".vert";
	inline static std::string_view s_fragExt = ".frag";

	EnumArray<Shader::Type, bytearray> codeMap;
	std::array<vk::ShaderModule, std::size_t(Shader::Type::eCOUNT_)> shaders;

	static std::string extension(stdfs::path const& id);

	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();

	vk::ShaderModule module(Shader::Type type) const;

	std::map<Shader::Type, vk::ShaderModule> modules() const;

#if defined(LEVK_SHADER_COMPILER)
	bool glslToSpirV(stdfs::path const& id, bytearray& out_bytes);
	bool loadGlsl(stdfs::path const& id, Shader::Type type);
#endif
	bool loadAllSpirV();

#if defined(LEVK_RESOURCES_HOT_RELOAD)
	bool checkReload();
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
	std::vector<Span<u8>> spanRaws;
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

	bool update();
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	bool checkReload();
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
	};

	gfx::Geometry geo;
	Data vbo;
	Data ibo;

	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();

	bool update();

	void updateGeometry(Info& out_info, gfx::Geometry geometry);
};

struct Font::Impl : ImplBase, ILoadable, IReloadable
{
	std::array<Glyph, maxVal<u8>()> glyphs;
	res::Material::Inst material;
	Glyph blankGlyph;
	res::Texture sheet;

	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();

	bool update();
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	bool checkReload();
#endif

	void loadGlyphs(std::vector<Glyph> const& glyphData, bool bOverwrite);
	gfx::Geometry generate(Text const& text) const;
};

Shader::Impl* impl(Shader shader);
Sampler::Impl* impl(Sampler sampler);
Texture::Impl* impl(Texture texture);
Material::Impl* impl(Material material);
Mesh::Impl* impl(Mesh mesh);
Font::Impl* impl(Font font);
Model::Impl* impl(Model model);

Shader::Info* infoRW(Shader shader);
Sampler::Info* infoRW(Sampler sampler);
Texture::Info* infoRW(Texture texture);
Material::Info* infoRW(Material material);
Mesh::Info* infoRW(Mesh mesh);
Font::Info* infoRW(Font font);
Model::Info* infoRW(Model model);

#if defined(LEVK_EDITOR)
template <typename T>
io::PathTree<T> const& loaded();

template <>
io::PathTree<Shader> const& loaded<Shader>();
template <>
io::PathTree<Sampler> const& loaded<Sampler>();
template <>
io::PathTree<Texture> const& loaded<Texture>();
template <>
io::PathTree<Material> const& loaded<Material>();
template <>
io::PathTree<Mesh> const& loaded<Mesh>();
template <>
io::PathTree<Font> const& loaded<Font>();
template <>
io::PathTree<Model> const& loaded<Model>();
#endif

bool isLoading(GUID guid);

void init();
void update();
void waitIdle();
void deinit();

#if defined(LEVK_EDITOR)
template <typename T>
io::PathTree<T> const& loaded()
{
	static_assert(alwaysFalse<T>, "Invalid Type!");
}
#endif

struct Service final
{
	Service();
	~Service();
};
} // namespace le::res
