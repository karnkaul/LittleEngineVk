#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <core/gdata.hpp>
#include <core/jobs.hpp>
#include <core/std_types.hpp>
#include <engine/assets/asset_list.hpp>
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
		eExtractingData,
		eLoadingAssets,
		eWaitingForAssets,
		eTerminating,
		eError,
	};

	struct Info final
	{
		std::vector<AssetData<gfx::Shader>> shaders;
		std::vector<AssetData<gfx::Texture>> textures;
		std::vector<AssetData<gfx::Texture>> cubemaps;
		std::vector<AssetData<gfx::Material>> materials;
		std::vector<AssetData<gfx::Mesh>> meshes;
		std::vector<AssetData<gfx::Model>> models;
		std::vector<AssetData<gfx::Font>> fonts;

		void intersect(AssetList ids);

		AssetList exportList() const;
		bool isEmpty() const;
	};

public:
	static std::string const s_tName;

protected:
	struct Data final
	{
		std::atomic<size_t> idCount = 0;
		std::atomic<size_t> dataCount = 0;
	};

public:
	AssetList m_loaded;
	Info m_toLoad;

protected:
	GData m_manifest;
	Data m_data;
	std::vector<std::shared_ptr<HJob>> m_running;
	std::vector<Asset*> m_loading;
	std::mutex m_mutex;
	Resources::Semaphore m_semaphore;
	Status m_status = Status::eIdle;
	IOReader const* m_pReader = nullptr;
	bool m_bParsed = false;

public:
	AssetManifest(IOReader const& reader, stdfs::path const& id);
	~AssetManifest();

public:
	void start();
	Status update(bool bTerminate = false);
	AssetList parse();

	static void unload(AssetList const& list);

protected:
	void loadData();
	void loadAssets();
	bool eraseDone(bool bWaitingJobs);
	void addJobs(IndexedTask task);
	void addJobs(TResult<IndexedTask> task);
};
} // namespace le
