#pragma once
#include <filesystem>
#include <type_traits>
#include <core/atomic_counter.hpp>
#include <engine/resources/resource_types.hpp>

namespace le
{
namespace stdfs = std::filesystem;

namespace res
{
using Semaphore = Counter<s32>::Semaphore;

Semaphore acquire();

Shader load(stdfs::path const& id, Shader::CreateInfo createInfo);
TResult<Shader> findShader(Hash id);
Shader::Info const& info(Shader shader);
Status status(Shader shader);
bool unload(Shader shader);
bool unloadShader(Hash id);

Sampler load(stdfs::path const& id, Sampler::CreateInfo createInfo);
TResult<Sampler> findSampler(Hash id);
Sampler::Info const& info(Sampler sampler);
Status status(Sampler sampler);
bool unload(Sampler sampler);
bool unloadSampler(Hash id);

Texture load(stdfs::path const& id, Texture::CreateInfo createInfo);
TResult<Texture> findTexture(Hash id);
Texture::Info const& info(Texture texture);
Status status(Texture texture);
bool unload(Texture texture);
bool unloadTexture(Hash id);

Material load(stdfs::path const& id, Material::CreateInfo createInfo);
TResult<Material> findMaterial(Hash id);
Material::Info const& info(Material material);
Status status(Material material);
bool unload(Material material);
bool unloadMaterial(Hash id);

bool unload(Hash id);

template <typename T>
T load(stdfs::path const& id, typename T::CreateInfo createInfo)
{
	if constexpr (std::is_same_v<T, Shader> || std::is_same_v<T, Sampler> || std::is_same_v<T, Texture>)
	{
		return load(id, std::move(createInfo));
	}
	else
	{
		static_assert(alwaysFalse<T>, "Invalid type!");
	}
}

template <typename T>
TResult<T> find(Hash id)
{
	if constexpr (std::is_same_v<T, Shader>)
	{
		return findShader(id);
	}
	else if constexpr (std::is_same_v<T, Sampler>)
	{
		return findSampler(id);
	}
	else if constexpr (std::is_same_v<T, Texture>)
	{
		return findTexture(id);
	}
	else
	{
		static_assert(alwaysFalse<T>, "Invalid type!");
	}
}
} // namespace res
} // namespace le
