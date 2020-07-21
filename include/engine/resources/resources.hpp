#pragma once
#include <filesystem>
#include <type_traits>
#include <core/atomic_counter.hpp>
#include <engine/resources/resource_types.hpp>

namespace le
{
namespace stdfs = std::filesystem;
}

namespace le::res
{
using Semaphore = Counter<s32>::Semaphore;

///
/// \brief Acquire a semaphore to block deinit/unload until (all have been) released
///
Semaphore acquire();

///
/// \brief Load new Shader
///
Shader load(stdfs::path const& id, Shader::CreateInfo createInfo);
///
/// \brief Find a loaded Shader
///
TResult<Shader> findShader(Hash id);
///
/// \brief Obtain Info for a loaded Shader
///
Shader::Info const& info(Shader shader);
///
/// \brief Obtain Status for a loaded Shader
///
Status status(Shader shader);
///
/// \brief Unload a loaded Shader
///
bool unload(Shader shader);
///
/// \brief Unload a loaded Shader
///
bool unloadShader(Hash id);

///
/// \brief Load new Sampler
///
Sampler load(stdfs::path const& id, Sampler::CreateInfo createInfo);
///
/// \brief Find a loaded Sampler
///
TResult<Sampler> findSampler(Hash id);
///
/// \brief Obtain Info for a loaded Sampler
///
Sampler::Info const& info(Sampler sampler);
///
/// \brief Obtain Status for a loaded Sampler
///
Status status(Sampler sampler);
///
/// \brief Unload a loaded Sampler
///
bool unload(Sampler sampler);
///
/// \brief Unload a loaded Sampler
///
bool unloadSampler(Hash id);

///
/// \brief Load new Texture
///
Texture load(stdfs::path const& id, Texture::CreateInfo createInfo);
///
/// \brief Find a loaded Texture
///
TResult<Texture> findTexture(Hash id);
///
/// \brief Obtain Info for a loaded Texture
///
Texture::Info const& info(Texture texture);
///
/// \brief Obtain Status for a loaded Texture
///
Status status(Texture texture);
///
/// \brief Unload a loaded Texture
///
bool unload(Texture texture);
///
/// \brief Unload a loaded Texture
///
bool unloadTexture(Hash id);

///
/// \brief Load new Material
///
Material load(stdfs::path const& id, Material::CreateInfo createInfo);
///
/// \brief Find a loaded Material
///
TResult<Material> findMaterial(Hash id);
///
/// \brief Obtain Info for a loaded Material
///
Material::Info const& info(Material material);
///
/// \brief Obtain Status for a loaded Material
///
Status status(Material material);
///
/// \brief Unload a loaded Material
///
bool unload(Material material);
///
/// \brief Unload a loaded Material
///
bool unloadMaterial(Hash id);

///
/// \brief Load new Mesh
///
Mesh load(stdfs::path const& id, Mesh::CreateInfo createInfo);
///
/// \brief Find a loaded Mesh
///
TResult<Mesh> findMesh(Hash id);
///
/// \brief Obtain Info for a loaded Mesh
///
Mesh::Info const& info(Mesh mesh);
///
/// \brief Obtain Status for a loaded Mesh
///
Status status(Mesh mesh);
///
/// \brief Unload a loaded Mesh
///
bool unload(Mesh mesh);
///
/// \brief Unload a loaded Mesh
///
bool unloadMesh(Hash id);

///
/// \brief Load new Font
///
Font load(stdfs::path const& id, Font::CreateInfo createInfo);
///
/// \brief Find a loaded Font
///
TResult<Font> findFont(Hash id);
///
/// \brief Obtain Info for a loaded Font
///
Font::Info const& info(Font font);
///
/// \brief Obtain Status for a loaded Font
///
Status status(Font font);
///
/// \brief Unload a loaded Font
///
bool unload(Font font);
///
/// \brief Unload a loaded Font
///
bool unloadFont(Hash id);

///
/// \brief Load new Model
///
Model load(stdfs::path const& id, Model::CreateInfo createInfo);
///
/// \brief Find a loaded Model
///
TResult<Model> findModel(Hash id);
///
/// \brief Obtain Info for a loaded Model
///
Model::Info const& info(Model model);
///
/// \brief Obtain Status for a loaded Model
///
Status status(Model model);
///
/// \brief Unload a loaded Model
///
bool unload(Model model);
///
/// \brief Unload a loaded Model
///
bool unloadModel(Hash id);

///
/// \brief Unload a loaded Resource
///
bool unload(Hash id);

template <typename T>
T load(stdfs::path const& id, typename T::CreateInfo createInfo)
{
	if constexpr (
		std::is_same_v<
			T,
			Shader> || std::is_same_v<T, Sampler> || std::is_same_v<T, Texture> || std::is_same_v<T, Material> || std::is_same_v<T, Mesh> || std::is_same_v<T, Font> || std::is_same_v<T, Model>)
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
	else if constexpr (std::is_same_v<T, Material>)
	{
		return findMaterial(id);
	}
	else if constexpr (std::is_same_v<T, Mesh>)
	{
		return findMesh(id);
	}
	else if constexpr (std::is_same_v<T, Font>)
	{
		return findFont(id);
	}
	else if constexpr (std::is_same_v<T, Model>)
	{
		return findModel(id);
	}
	else
	{
		static_assert(alwaysFalse<T>, "Invalid type!");
	}
}
} // namespace le::res
