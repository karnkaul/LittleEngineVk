#pragma once
#include <atomic>
#include <memory>
#include <core/gdata.hpp>
#include <core/reader.hpp>
#include <core/std_types.hpp>
#include <core/tasks.hpp>
#include <core/utils.hpp>
#include <engine/resources/asset_list.hpp>
#include <engine/resources/resources.hpp>

namespace le
{
template <typename T>
struct ResourceData final
{
	typename T::CreateInfo createInfo;
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
		std::vector<ResourceData<res::Shader>> shaders;
		std::vector<ResourceData<res::Texture>> textures;
		std::vector<ResourceData<res::Texture>> cubemaps;
		std::vector<ResourceData<res::Material>> materials;
		std::vector<ResourceData<res::Mesh>> meshes;
		std::vector<ResourceData<res::Model>> models;
		std::vector<ResourceData<res::Font>> fonts;

		void intersect(AssetList ids);

		AssetList exportList() const;
		bool isEmpty() const;
	};

public:
	static std::string const s_tName;

protected:
	struct Data final
	{
		std::atomic<std::size_t> idCount = 0;
		std::atomic<std::size_t> dataCount = 0;
	};

public:
	AssetList m_loaded;
	Info m_toLoad;

protected:
	GData m_manifest;
	Data m_data;
	std::vector<std::shared_ptr<tasks::Handle>> m_running;
	std::vector<res::GUID> m_loading;
	Lockable<std::mutex> m_mutex;
	res::Semaphore m_semaphore;
	Status m_status = Status::eIdle;
	io::Reader const* m_pReader = nullptr;
	bool m_bParsed = false;

public:
	AssetManifest(io::Reader const& reader, stdfs::path const& id);
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
	void addJobs(std::vector<std::shared_ptr<tasks::Handle>> handles);
};
} // namespace le
