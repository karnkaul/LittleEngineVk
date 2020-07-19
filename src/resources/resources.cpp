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
#include <levk_impl.hpp>

namespace le
{
namespace
{
using namespace resources;

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
	TImpl impl;
	impl.id = id;
	impl.guid = guid;
	if (impl.make(out_createInfo))
	{
		T resource;
		resource.guid = guid;
		out_createInfo.info.id = impl.id = id;
		impl.guid = guid;
		TResource<T, TImpl> tResource{std::move(out_createInfo.info), resource, std::move(impl)};
		auto lock = out_map.mutex.template lock<std::unique_lock>();
		auto const guid = resource.guid;
		out_map.ids[id] = guid;
		bool const bLoading = tResource.impl.status == Status::eLoading;
		out_map.resources.emplace(guid, std::move(tResource));
		if (bLoading)
		{
			out_map.loading[guid] = &out_map.resources.find(guid).payload->impl;
			LOG_I("== [{}] [{}] loading...", T::s_tName, id.generic_string());
		}
		else
		{
			out_map.resources.find(guid).payload->impl.bLoadedOnce = true;
			LOG_I("== [{}] [{}] loaded", T::s_tName, id.generic_string());
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
		return &pResource->impl;
	}
	return nullptr;
}

template <typename T, typename TImpl>
typename T::Info const& findInfo(Map<T, TImpl>& out_map, GUID guid)
{
	static typename T::Info const s_default;
	auto lock = out_map.mutex.template lock<std::shared_lock>();
	auto [pResource, bResult] = out_map.resources.find(guid);
	if (bResult)
	{
		return pResource->info;
	}
	return s_default;
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
			LOG_I("-- [{}] [{}] unloaded", T::s_tName, tResource->impl.id.generic_string());
			tResource->impl.release();
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
		LOG_I("-- [{}] [{}] unloaded", T::s_tName, tResource->impl.id.generic_string());
		tResource->impl.release();
		out_map.ids.erase(tResource->impl.id);
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
					pImpl->onReload();
				}
#endif
				pImpl->bLoadedOnce = true;
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
			if (tResource.impl.checkReload())
			{
				if constexpr (std::is_base_of_v<ILoadable, TImpl>)
				{
					out_map.loading[guid] = &tResource.impl;
				}
				else
				{
					tResource.impl.onReload();
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
		LOG_I("-- [{}] [{}] unloaded", T::s_tName, tResource.impl.id.generic_string());
		tResource.impl.release();
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

template <typename T, typename TImpl>
std::vector<T> loaded(Map<T, TImpl> const& map)
{
	std::vector<T> ret;
	auto lock = map.mutex.template lock<std::shared_lock>();
	ret.reserve(map.resources.m_map.size());
	for (auto& [_, tResource] : map.resources.m_map)
	{
		ret.push_back(tResource.resource);
	}
	return ret;
}

template <typename T, typename TImpl>
bool isLoading(Map<T, TImpl> const& map, GUID guid)
{
	auto lock = map.mutex.template lock<std::shared_lock>();
	return map.loading.find(guid) != map.loading.end();
}

Map<Shader, Shader::Impl> g_shaders;
Map<Sampler, Sampler::Impl> g_samplers;
Map<Texture, Texture::Impl> g_textures;

bool g_bInit = false;
Counter<s32> g_counter;
} // namespace

resources::Semaphore resources::acquire()
{
	return Semaphore(g_counter);
}

resources::Shader resources::load(stdfs::path const& id, Shader::CreateInfo createInfo)
{
	return g_bInit ? make(g_shaders, createInfo, id) : Shader();
}

TResult<resources::Shader> resources::findShader(Hash id)
{
	return g_bInit ? find(g_shaders, id) : TResult<Shader>();
}

resources::Shader::Info const& resources::info(Shader shader)
{
	static Shader::Info const s_default;
	return g_bInit ? findInfo(g_shaders, shader.guid) : s_default;
}

Status resources::status(Shader shader)
{
	return g_bInit ? status(g_shaders, shader.guid) : Status::eIdle;
}

bool resources::unload(Shader shader)
{
	return g_bInit ? unload(g_shaders, shader.guid) : false;
}

bool resources::unloadShader(Hash id)
{
	return g_bInit ? unload(g_shaders, id) : false;
}

resources::Sampler resources::load(stdfs::path const& id, Sampler::CreateInfo createInfo)
{
	return g_bInit ? make(g_samplers, createInfo, id) : Sampler();
}

TResult<resources::Sampler> resources::findSampler(Hash id)
{
	return g_bInit ? find(g_samplers, id) : TResult<Sampler>();
}

resources::Sampler::Info const& resources::info(Sampler sampler)
{
	static Sampler::Info const s_default;
	return g_bInit ? findInfo(g_samplers, sampler.guid) : s_default;
}

resources::Status resources::status(Sampler sampler)
{
	return g_bInit ? status(g_samplers, sampler.guid) : Status::eIdle;
}

bool resources::unload(Sampler sampler)
{
	return g_bInit ? unload(g_samplers, sampler.guid) : false;
}

bool resources::unloadSampler(Hash id)
{
	return g_bInit ? unload(g_samplers, id) : false;
}

resources::Texture resources::load(stdfs::path const& id, Texture::CreateInfo createInfo)
{
	return g_bInit ? make(g_textures, createInfo, id) : Texture();
}

TResult<resources::Texture> resources::findTexture(Hash id)
{
	return g_bInit ? find(g_textures, id) : TResult<Texture>();
}

resources::Texture::Info const& resources::info(Texture texture)
{
	static Texture::Info const s_default;
	return g_bInit ? findInfo(g_textures, texture.guid) : s_default;
}

resources::Status resources::status(Texture texture)
{
	return g_bInit ? status(g_textures, texture.guid) : Status::eIdle;
}

bool resources::unload(Texture texture)
{
	return g_bInit ? unload(g_textures, texture.guid) : false;
}

bool resources::unloadTexture(Hash id)
{
	return g_bInit ? unload(g_textures, id) : false;
}

resources::Shader::Impl* resources::impl(Shader shader)
{
	return g_bInit ? findImpl(g_shaders, shader.guid) : nullptr;
}

resources::Sampler::Impl* resources::impl(Sampler sampler)
{
	return g_bInit ? findImpl(g_samplers, sampler.guid) : nullptr;
}

resources::Texture::Impl* resources::impl(Texture texture)
{
	return g_bInit ? findImpl(g_textures, texture.guid) : nullptr;
}

std::vector<Shader> resources::loadedShaders()
{
	return g_bInit ? loaded(g_shaders) : std::vector<Shader>();
}

std::vector<Sampler> resources::loadedSamplers()
{
	return g_bInit ? loaded(g_samplers) : std::vector<Sampler>();
}

std::vector<Texture> resources::loadedTextures()
{
	return g_bInit ? loaded(g_textures) : std::vector<Texture>();
}

bool resources::isLoading(GUID guid)
{
	if (g_bInit)
	{
		return isLoading(g_shaders, guid) || isLoading(g_samplers, guid) || isLoading(g_textures, guid);
	}
	return false;
}

void resources::init()
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
			info.info.mode = Sampler::Mode::eClampEdge;
			info.info.min = Sampler::Filter::eNearest;
			info.info.mip = Sampler::Filter::eNearest;
			load("samplers/font", std::move(info));
			LOG_I("[le::resources] initialised");
		}
		{
			Texture::CreateInfo info;
			info.info.type = Texture::Type::e2D;
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
			info.info.type = Texture::Type::eCube;
			load("cubemaps/blank", std::move(info));
		}
	}
}

void resources::update()
{
	update(g_shaders);
	update(g_samplers);
	update(g_textures);
}

void resources::waitIdle()
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
}

void resources::deinit()
{
	if (g_bInit)
	{
		g_bInit = false;
		waitIdle();
		release(g_shaders);
		release(g_samplers);
		release(g_textures);
		LOG_I("[le::resources] deinitialised");
	}
}

resources::Service::Service()
{
	init();
}

resources::Service::~Service()
{
	deinit();
}
} // namespace le
