#pragma once
#include <atomic>
#include <memory>
#include <vector>
#include <core/gdata.hpp>
#include <core/jobs.hpp>
#include <core/std_types.hpp>
#include <core/io.hpp>
#include <engine/assets/resources.hpp>
#include <engine/gfx/font.hpp>
#include <engine/gfx/mesh.hpp>
#include <engine/gfx/model.hpp>
#include <engine/gfx/shader.hpp>
#include <engine/gfx/texture.hpp>

namespace le
{
template <typename T>
struct AssetData
{
	typename T::Info info;
	stdfs::path id;
};

class AssetManifest
{
public:
	enum class Status : s8
	{
		eIdle,
		eExtractingIDs,
		eExtractingData,
		eLoadingAssets,
		eTerminating,
		eError,
	};

public:
	static std::string const s_tName;

public:
	struct Info final
	{
		std::vector<AssetData<gfx::Shader>> shaders;
		std::vector<AssetData<gfx::Texture>> textures;
		std::vector<AssetData<gfx::Texture>> cubemaps;
		std::vector<AssetData<gfx::Material>> materials;
		std::vector<AssetData<gfx::Mesh>> meshes;
		std::vector<AssetData<gfx::Model>> models;
		std::vector<AssetData<gfx::Font>> fonts;

		Info operator*(Info const& rhs) const;
		Info operator-(Info const& rhs) const;

		bool isEmpty() const;
	};

protected:
	struct Data final
	{
		std::atomic<size_t> idCount = 0;
		std::atomic<size_t> dataCount = 0;
	};

public:
	Info m_cache;

protected:
	GData m_manifest;
	Info m_info;
	Data m_data;
	std::vector<std::shared_ptr<HJob>> m_running;
	std::vector<Resources::Semaphore> m_semaphores;
	Status m_status = Status::eIdle;
	IOReader const* m_pReader = nullptr;

public:
	AssetManifest(IOReader const& reader, stdfs::path const& id);
	~AssetManifest();

public:
	void start();
	Status update(bool bTerminate = false);

protected:
	void extractIDs();
	void extractData();
	void loadAssets();
	bool eraseDone();
	void addJobs(IndexedTask const& task);
};
} // namespace le
