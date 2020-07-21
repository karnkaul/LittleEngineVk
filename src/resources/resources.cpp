#include <array>
#include <atomic>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <core/log.hpp>
#include <core/map_store.hpp>
#include <core/threads.hpp>
#include <engine/resources/resources.hpp>
#include <gfx/vram.hpp>
#include <resources/resources_impl.hpp>
#include <resources/model_impl.hpp>
#include <levk_impl.hpp>

namespace le
{
namespace
{
using namespace res;

template <typename T, typename TImpl = void>
struct Map
{
	TMapStore<std::unordered_map<GUID, TResource<T, TImpl>>> resources;
	std::unordered_map<Hash, GUID> ids;
	std::unordered_map<GUID, TImpl*> loading;
	mutable Lockable<std::shared_mutex> mutex;
};

std::atomic<GUID::type> g_nextGUID;

template <typename T, typename TImpl>
T make(Map<T, TImpl>& out_map, typename T::CreateInfo& out_createInfo, stdfs::path const& id)
{
	GUID guid = ++g_nextGUID;
	std::unique_ptr<TImpl> uImpl = std::make_unique<TImpl>();
	typename T::Info info;
	uImpl->id = id;
	uImpl->guid = guid;
	LOGIF_W(id.empty(), "[{}] Empty resource ID!", T::s_tName);
	if (!id.empty() && uImpl->make(out_createInfo, info))
	{
		T resource;
		resource.guid = guid;
		info.id = uImpl->id = id;
		uImpl->guid = guid;
		TImpl* pImpl = uImpl.get();
		TResource<T, TImpl> tResource{std::move(info), resource, std::move(uImpl)};
		auto lock = out_map.mutex.template lock<std::unique_lock>();
		auto const guid = resource.guid;
		out_map.ids[id] = guid;
		out_map.resources.emplace(guid, std::move(tResource));
		if (std::is_base_of_v<ILoadable, TImpl> && pImpl->status != Status::eReady)
		{
			pImpl->status = Status::eLoading;
			out_map.loading[guid] = pImpl;
			LOG_I("++ [{}] [{}] [{}] loading...", guid, T::s_tName, id.generic_string());
		}
		else
		{
			pImpl->status = Status::eReady;
			LOG_I("== [{}] [{}] [{}] loaded", guid, T::s_tName, id.generic_string());
		}
		return resource;
	}
	return {};
}

template <typename T, typename TImpl>
TResult<T> find(Map<T, TImpl> const& map, GUID guid)
{
	auto lock = map.mutex.template lock<std::shared_lock>();
	auto [pResource, bResult] = map.resources.find(guid);
	if (bResult)
	{
		return T{pResource->resource};
	}
	return {};
}

template <typename T, typename TImpl>
TResult<T> find(Map<T, TImpl> const& map, Hash id)
{
	auto lock = map.mutex.template lock<std::shared_lock>();
	if (auto search = map.ids.find(id); search != map.ids.end())
	{
		auto [pResource, bResult] = map.resources.find(search->second);
		if (bResult)
		{
			return T{pResource->resource};
		}
	}
	return {};
}

template <typename T, typename TImpl>
TImpl* findImpl(Map<T, TImpl>& map, GUID guid)
{
	auto lock = map.mutex.template lock<std::shared_lock>();
	auto [pResource, bResult] = map.resources.find(guid);
	if (bResult)
	{
		return pResource->uImpl.get();
	}
	return nullptr;
}

template <typename T, typename TImpl>
typename T::Info const& findInfo(Map<T, TImpl>& out_map, GUID guid)
{
	static typename T::Info const s_default{};
	auto lock = out_map.mutex.template lock<std::shared_lock>();
	auto [pResource, bResult] = out_map.resources.find(guid);
	if (bResult)
	{
		return pResource->info;
	}
	return s_default;
}

template <typename T, typename TImpl>
typename T::Info* findInfoRW(Map<T, TImpl>& out_map, GUID guid)
{
	auto lock = out_map.mutex.template lock<std::shared_lock>();
	auto [pResource, bResult] = out_map.resources.find(guid);
	if (bResult)
	{
		return &pResource->info;
	}
	return nullptr;
}

template <typename T, typename TImpl>
bool unload(Map<T, TImpl>& out_map, Hash id)
{
	auto lock = out_map.mutex.template lock<std::unique_lock>();
	if (auto search = out_map.ids.find(id); search != out_map.ids.end())
	{
		auto const guid = search->second;
		out_map.ids.erase(search);
		auto [tResource, bResult] = out_map.resources.find(guid);
		if (bResult)
		{
			LOG_I("-- [{}] [{}] [{}] unloaded", guid, T::s_tName, tResource->uImpl->id.generic_string());
			lock.unlock();
			tResource->uImpl->release();
			lock.lock();
		}
		return out_map.resources.unload(guid);
	}
	return false;
}

template <typename T, typename TImpl>
bool unload(Map<T, TImpl>& out_map, GUID guid)
{
	auto lock = out_map.mutex.template lock<std::unique_lock>();
	auto [tResource, bResult] = out_map.resources.find(guid);
	if (bResult)
	{
		LOG_I("-- [{}] [{}] [{}] unloaded", guid, T::s_tName, tResource->uImpl->id.generic_string());
		out_map.ids.erase(tResource->uImpl->id);
		lock.unlock();
		tResource->uImpl->release();
		lock.lock();
	}
	return out_map.resources.unload(guid);
}

template <typename T, typename TImpl>
void update(Map<T, TImpl>& out_map)
{
	if constexpr (std::is_base_of_v<ILoadable, TImpl>)
	{
		auto lock = out_map.mutex.template lock<std::shared_lock>();
		for (auto iter = out_map.loading.begin(); iter != out_map.loading.end();)
		{
			auto& [guid, pImpl] = *iter;
			if (pImpl->update())
			{
#if defined(LEVK_RESOURCES_HOT_RELOAD)
				if (pImpl->bLoadedOnce)
				{
					LOG_D("== [{}] [{}] [{}] reloaded", guid, T::s_tName, pImpl->id.generic_string());
					if constexpr (std::is_base_of_v<IReloadable, TImpl>)
					{
						pImpl->onReload();
					}
				}
#endif
				pImpl->bLoadedOnce = true;
				pImpl->status = Status::eReady;
				iter = out_map.loading.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	if constexpr (std::is_base_of_v<IReloadable, TImpl>)
	{
		auto lock = out_map.mutex.template lock<std::shared_lock>();
		for (auto& [guid, tResource] : out_map.resources.m_map)
		{
			if (tResource.uImpl->checkReload())
			{
				if constexpr (std::is_base_of_v<ILoadable, TImpl>)
				{
					LOG_D("++ [{}] [{}] [{}] reloading...", guid, T::s_tName, tResource.uImpl->id.generic_string());
					tResource.uImpl->status = Status::eReloading;
					out_map.loading[guid] = tResource.uImpl.get();
				}
				else
				{
					LOG_D("== [{}] [{}] [{}] reloaded", guid, T::s_tName, tResource.uImpl->id.generic_string());
					tResource.uImpl->onReload();
				}
			}
		}
	}
#endif
}

template <typename T, typename TImpl>
void waitLoading(Map<T, TImpl>& out_map)
{
	bool bEmpty = false;
	while (!bEmpty)
	{
		{
			auto lock = out_map.mutex.template lock<std::unique_lock>();
			bEmpty = out_map.loading.empty();
		}
		if (!bEmpty)
		{
			gfx::vram::update();
			update(out_map);
			threads::sleep();
		}
	}
}

template <typename T, typename TImpl>
void release(Map<T, TImpl>& out_map)
{
	waitLoading(out_map);
	auto lock = out_map.mutex.template lock<std::unique_lock>();
	for (auto iter = out_map.resources.m_map.begin(); iter != out_map.resources.m_map.end();)
	{
		auto& [guid, tResource] = *iter;
		LOG_I("-- [{}] [{}] [{}] unloaded", guid, T::s_tName, tResource.uImpl->id.generic_string());
		lock.unlock();
		tResource.uImpl->release();
		lock.lock();
		iter = out_map.resources.m_map.erase(iter);
	}
	out_map.resources.unloadAll();
	out_map.ids.clear();
	out_map.loading.clear();
}

template <typename T, typename TImpl>
Status status(Map<T, TImpl>& map, GUID guid)
{
	if (auto pImpl = findImpl<T, TImpl>(map, guid))
	{
		return pImpl->status;
	}
	return Status::eIdle;
}

#if defined(LEVK_EDITOR)
template <typename T, typename TImpl>
TreeView<T> const& loaded(Map<T, TImpl> const& map)
{
	static TreeView<T> s_ret;
	static GUID::type s_guid;
	auto lock = map.mutex.template lock<std::shared_lock>();
	if (s_ret.entries.size() != map.resources.m_map.size() || g_nextGUID > s_guid)
	{
		s_guid = g_nextGUID;
		s_ret.entries.clear();
		for (auto& [_, tResource] : map.resources.m_map)
		{
			s_ret.add(tResource.uImpl->id, tResource.resource);
		}
	}
	return s_ret;
}
#endif

template <typename T, typename TImpl>
bool isLoading(Map<T, TImpl> const& map, GUID guid)
{
	auto lock = map.mutex.template lock<std::shared_lock>();
	return map.loading.find(guid) != map.loading.end();
}

Map<Shader, Shader::Impl> g_shaders;
Map<Sampler, Sampler::Impl> g_samplers;
Map<Texture, Texture::Impl> g_textures;
Map<Material, Material::Impl> g_materials;
Map<Mesh, Mesh::Impl> g_meshes;
Map<Font, Font::Impl> g_fonts;
Map<Model, Model::Impl> g_models;

bool g_bInit = false;
Counter<s32> g_counter;
} // namespace

res::Semaphore res::acquire()
{
	return Semaphore(g_counter);
}

res::Shader res::load(stdfs::path const& id, Shader::CreateInfo createInfo)
{
	return g_bInit ? make(g_shaders, createInfo, id) : Shader();
}

res::Sampler res::load(stdfs::path const& id, Sampler::CreateInfo createInfo)
{
	return g_bInit ? make(g_samplers, createInfo, id) : Sampler();
}

res::Texture res::load(stdfs::path const& id, Texture::CreateInfo createInfo)
{
	return g_bInit ? make(g_textures, createInfo, id) : Texture();
}

res::Material res::load(stdfs::path const& id, Material::CreateInfo createInfo)
{
	return g_bInit ? make(g_materials, createInfo, id) : Material();
}

res::Mesh res::load(stdfs::path const& id, Mesh::CreateInfo createInfo)
{
	return g_bInit ? make(g_meshes, createInfo, id) : Mesh();
}

res::Font res::load(stdfs::path const& id, Font::CreateInfo createInfo)
{
	return g_bInit ? make(g_fonts, createInfo, id) : Font();
}

TResult<res::Shader> res::findShader(Hash id)
{
	return g_bInit ? find(g_shaders, id) : TResult<Shader>();
}

TResult<res::Sampler> res::findSampler(Hash id)
{
	return g_bInit ? find(g_samplers, id) : TResult<Sampler>();
}

TResult<res::Texture> res::findTexture(Hash id)
{
	return g_bInit ? find(g_textures, id) : TResult<Texture>();
}

TResult<res::Material> res::findMaterial(Hash id)
{
	return g_bInit ? find(g_materials, id) : TResult<Material>();
}

TResult<res::Mesh> res::findMesh(Hash id)
{
	return g_bInit ? find(g_meshes, id) : TResult<Mesh>();
}

TResult<res::Font> res::findFont(Hash id)
{
	return g_bInit ? find(g_fonts, id) : TResult<Font>();
}

res::Shader::Info const& res::info(Shader shader)
{
	static Shader::Info const s_default{};
	return g_bInit ? findInfo(g_shaders, shader.guid) : s_default;
}

res::Sampler::Info const& res::info(Sampler sampler)
{
	static Sampler::Info const s_default{};
	return g_bInit ? findInfo(g_samplers, sampler.guid) : s_default;
}

res::Texture::Info const& res::info(Texture texture)
{
	static Texture::Info const s_default{};
	return g_bInit ? findInfo(g_textures, texture.guid) : s_default;
}

res::Material::Info const& res::info(Material material)
{
	static Material::Info const s_default{};
	return g_bInit ? findInfo(g_materials, material.guid) : s_default;
}

res::Mesh::Info const& res::info(Mesh mesh)
{
	static Mesh::Info const s_default{};
	return g_bInit ? findInfo(g_meshes, mesh.guid) : s_default;
}

res::Font::Info const& res::info(Font font)
{
	static Font::Info const s_default{};
	return g_bInit ? findInfo(g_fonts, font.guid) : s_default;
}

Status res::status(Shader shader)
{
	return g_bInit ? status(g_shaders, shader.guid) : Status::eIdle;
}

res::Status res::status(Sampler sampler)
{
	return g_bInit ? status(g_samplers, sampler.guid) : Status::eIdle;
}

res::Status res::status(Texture texture)
{
	return g_bInit ? status(g_textures, texture.guid) : Status::eIdle;
}

res::Status res::status(Material material)
{
	return g_bInit ? status(g_materials, material.guid) : Status::eIdle;
}

res::Status res::status(Mesh mesh)
{
	return g_bInit ? status(g_meshes, mesh.guid) : Status::eIdle;
}

res::Status res::status(Font font)
{
	return g_bInit ? status(g_fonts, font.guid) : Status::eIdle;
}

bool res::unload(Shader shader)
{
	return g_bInit ? unload(g_shaders, shader.guid) : false;
}

bool res::unload(Sampler sampler)
{
	return g_bInit ? unload(g_samplers, sampler.guid) : false;
}

bool res::unload(Texture texture)
{
	return g_bInit ? unload(g_textures, texture.guid) : false;
}

bool res::unload(Material material)
{
	return g_bInit ? unload(g_materials, material.guid) : false;
}

bool res::unload(Mesh mesh)
{
	return g_bInit ? unload(g_meshes, mesh.guid) : false;
}

bool res::unload(Font font)
{
	return g_bInit ? unload(g_fonts, font.guid) : false;
}

bool res::unloadShader(Hash id)
{
	return g_bInit ? unload(g_shaders, id) : false;
}

bool res::unloadSampler(Hash id)
{
	return g_bInit ? unload(g_samplers, id) : false;
}

bool res::unloadTexture(Hash id)
{
	return g_bInit ? unload(g_textures, id) : false;
}

bool res::unloadMaterial(Hash id)
{
	return g_bInit ? unload(g_materials, id) : false;
}

bool res::unloadMesh(Hash id)
{
	return g_bInit ? unload(g_meshes, id) : false;
}

bool res::unloadFont(Hash id)
{
	return g_bInit ? unload(g_fonts, id) : false;
}

res::Model res::load(stdfs::path const& id, Model::CreateInfo createInfo)
{
	return g_bInit ? make(g_models, createInfo, id) : res::Model();
}

TResult<Model> res::findModel(Hash id)
{
	return g_bInit ? find(g_models, id) : TResult<Model>();
}

Model::Info const& res::info(Model model)
{
	static Model::Info const s_default{};
	return g_bInit ? findInfo(g_models, model.guid) : s_default;
}

Status res::status(Model model)
{
	return g_bInit ? status(g_models, model.guid) : Status::eIdle;
}

bool res::unload(Model model)
{
	return g_bInit ? unload(g_models, model.guid) : false;
}

bool res::unloadModel(Hash id)
{
	return g_bInit ? unload(g_models, id) : false;
}

res::Shader::Impl* res::impl(Shader shader)
{
	return g_bInit ? findImpl(g_shaders, shader.guid) : nullptr;
}

res::Sampler::Impl* res::impl(Sampler sampler)
{
	return g_bInit ? findImpl(g_samplers, sampler.guid) : nullptr;
}

res::Texture::Impl* res::impl(Texture texture)
{
	return g_bInit ? findImpl(g_textures, texture.guid) : nullptr;
}

res::Material::Impl* res::impl(Material material)
{
	return g_bInit ? findImpl(g_materials, material.guid) : nullptr;
}

res::Mesh::Impl* res::impl(Mesh mesh)
{
	return g_bInit ? findImpl(g_meshes, mesh.guid) : nullptr;
}

res::Font::Impl* res::impl(Font font)
{
	return g_bInit ? findImpl(g_fonts, font.guid) : nullptr;
}

res::Model::Impl* res::impl(Model model)
{
	return g_bInit ? findImpl(g_models, model.guid) : nullptr;
}

Shader::Info* res::infoRW(Shader shader)
{
	return g_bInit ? findInfoRW(g_shaders, shader.guid) : nullptr;
}

Sampler::Info* res::infoRW(Sampler sampler)
{
	return g_bInit ? findInfoRW(g_samplers, sampler.guid) : nullptr;
}

Texture::Info* res::infoRW(Texture texture)
{
	return g_bInit ? findInfoRW(g_textures, texture.guid) : nullptr;
}

Material::Info* res::infoRW(Material material)
{
	return g_bInit ? findInfoRW(g_materials, material.guid) : nullptr;
}

Mesh::Info* res::infoRW(Mesh mesh)
{
	return g_bInit ? findInfoRW(g_meshes, mesh.guid) : nullptr;
}

Font::Info* res::infoRW(Font font)
{
	return g_bInit ? findInfoRW(g_fonts, font.guid) : nullptr;
}

Model::Info* res::infoRW(Model model)
{
	return g_bInit ? findInfoRW(g_models, model.guid) : nullptr;
}

#if defined(LEVK_EDITOR)
TreeView<Shader> const& res::loadedShaders()
{
	static TreeView<Shader> const s_default{};
	return g_bInit ? loaded(g_shaders) : s_default;
}

TreeView<Sampler> const& res::loadedSamplers()
{
	static TreeView<Sampler> const s_default{};
	return g_bInit ? loaded(g_samplers) : s_default;
}

TreeView<Texture> const& res::loadedTextures()
{
	static TreeView<Texture> const s_default{};
	return g_bInit ? loaded(g_textures) : s_default;
}

TreeView<Material> const& res::loadedMaterials()
{
	static TreeView<Material> const s_default{};
	return g_bInit ? loaded(g_materials) : s_default;
}

TreeView<Mesh> const& res::loadedMeshes()
{
	static TreeView<Mesh> const s_default{};
	return g_bInit ? loaded(g_meshes) : s_default;
}

TreeView<Font> const& res::loadedFonts()
{
	static TreeView<Font> const s_default{};
	return g_bInit ? loaded(g_fonts) : s_default;
}

TreeView<Model> const& res::loadedModels()
{
	static TreeView<Model> const s_default{};
	return g_bInit ? loaded(g_models) : s_default;
}
#endif

bool res::unload(Hash id)
{
	return g_bInit ? unloadShader(id) || unloadSampler(id) || unloadTexture(id) || unloadMaterial(id) || unloadMesh(id) || unloadFont(id) || unloadModel(id)
				   : false;
}

bool res::isLoading(GUID guid)
{
	if (g_bInit)
	{
		return isLoading(g_shaders, guid) || isLoading(g_samplers, guid) || isLoading(g_textures, guid) || isLoading(g_materials, guid)
			   || isLoading(g_meshes, guid) || isLoading(g_fonts, guid) || isLoading(g_models, guid);
	}
	return false;
}

void res::init()
{
	if (!g_bInit)
	{
		g_bInit = true;
		constexpr static std::array<u8, 4> white1pxBytes = {0xff, 0xff, 0xff, 0xff};
		constexpr static std::array<u8, 4> black1pxBytes = {0x0, 0x0, 0x0, 0x0};
		auto semaphore = acquire();
		{
			Shader::CreateInfo info;
			static std::array const shaderIDs = {stdfs::path("shaders/uber.vert"), stdfs::path("shaders/uber.frag")};
			ASSERT(engine::reader().checkPresences(shaderIDs), "Uber Shader not found!");
			info.codeIDMap.at((std::size_t)Shader::Type::eVertex) = shaderIDs.at(0);
			info.codeIDMap.at((std::size_t)Shader::Type::eFragment) = shaderIDs.at(1);
			load("shaders/default", std::move(info));
		}
		{
			load("samplers/default", Sampler::CreateInfo());
			Sampler::CreateInfo info;
			info.mode = Sampler::Mode::eClampEdge;
			info.min = Sampler::Filter::eNearest;
			info.mip = Sampler::Filter::eNearest;
			load("samplers/font", std::move(info));
		}
		{
			Texture::CreateInfo info;
			info.type = Texture::Type::e2D;
			info.raws = {{Span<u8>(white1pxBytes), {1, 1}}};
			load("textures/white", info);
			info.raws.back().bytes = Span<u8>(black1pxBytes);
			load("textures/black", info);
		}
		{
			Texture::CreateInfo info;
			Texture::Raw b1px;
			b1px.bytes = Span<u8>(black1pxBytes);
			b1px.size = {1, 1};
			info.raws = {b1px, b1px, b1px, b1px, b1px, b1px};
			info.type = Texture::Type::eCube;
			load("cubemaps/blank", std::move(info));
		}
		{
			load("materials/default", Material::CreateInfo());
		}
		{
			Mesh::CreateInfo info;
			info.geometry = gfx::createCube();
			load("meshes/cube", std::move(info));
		}
		{
			Font::CreateInfo fontInfo;
			auto [str, bResult] = engine::reader().getString("fonts/default.json");
			ASSERT(bResult, "Default font not found!");
			GData fontData;
			if (fontData.read(std::move(str)))
			{
				fontInfo.deserialise(fontData);
				auto [img, bImg] = engine::reader().getBytes(stdfs::path("fonts") / fontInfo.sheetID);
				ASSERT(bImg, "Default font not found!");
				fontInfo.image = std::move(img);
				auto [material, bMaterial] = res::findMaterial(fontInfo.materialID);
				if (bMaterial)
				{
					fontInfo.material.material = material;
				}
				load("fonts/default", std::move(fontInfo));
			}
			else
			{
				LOG_E("[le::resources] Failed to create default font!");
			}
		}
		LOG_I("[le::resources] initialised");
	}
}

void res::update()
{
	update(g_shaders);
	update(g_samplers);
	update(g_textures);
	update(g_materials);
	update(g_meshes);
	update(g_fonts);
	update(g_models);
}

void res::waitIdle()
{
	constexpr Time timeout = 5s;
	Time elapsed;
	Time const start = Time::elapsed();
	while (!g_counter.isZero(true) && elapsed < timeout)
	{
		elapsed = Time::elapsed() - start;
		threads::sleep();
	}
	bool bTimeout = elapsed > timeout;
	ASSERT(!bTimeout, "Timeout waiting for Resources! Expect a crash");
	LOGIF_E(bTimeout, "[le::resources] Timeout waiting for Resources! Expect crashes/hangs!");
	waitLoading(g_shaders);
	waitLoading(g_samplers);
	waitLoading(g_textures);
	waitLoading(g_materials);
	waitLoading(g_meshes);
	waitLoading(g_fonts);
	waitLoading(g_models);
}

void res::deinit()
{
	if (g_bInit)
	{
		waitIdle();
		release(g_shaders);
		release(g_samplers);
		release(g_textures);
		release(g_materials);
		release(g_meshes);
		release(g_fonts);
		release(g_models);
		g_bInit = false;
		LOG_I("[le::resources] deinitialised");
	}
}

res::Service::Service()
{
	init();
}

res::Service::~Service()
{
	deinit();
}
} // namespace le
