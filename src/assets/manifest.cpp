#include <algorithm>
#include <sstream>
#include <utility>
#include <core/assert.hpp>
#include <core/jobs.hpp>
#include <core/log.hpp>
#include <core/threads.hpp>
#include <core/utils.hpp>
#include <assets/manifest.hpp>
#include <levk_impl.hpp>

namespace le
{
using namespace gfx;

namespace
{
template <typename T>
void intersectTlist(std::vector<AssetData<T>>& out_data, std::vector<stdfs::path> const& src)
{
	auto iter = std::remove_if(out_data.begin(), out_data.end(), [&src](auto const& data) {
		return std::find_if(src.begin(), src.end(), [&data](auto const& id) { return data.id == id; }) == src.end();
	});
	out_data.erase(iter, out_data.end());
}

template <typename T>
std::vector<stdfs::path> exportTList(std::vector<AssetData<T>> const& src)
{
	std::vector<stdfs::path> ret;
	for (auto const& data : src)
	{
		ret.push_back(data.id);
	}
	return ret;
}

template <typename... T>
bool empty(std::vector<T> const&... out_vecs)
{
	return (out_vecs.empty() && ...);
}

template <typename T>
TResult<IndexedTask> loadTAssets(std::vector<AssetData<T>>& out_toLoad, std::vector<stdfs::path>& out_loaded,
								 std::vector<Asset*>& out_assets, std::mutex& mutex, std::string_view jobName)
{
	IndexedTask task;
	if (!out_toLoad.empty())
	{
		task.task = [&out_toLoad, &out_loaded, &out_assets, &mutex](size_t idx) {
			auto& asset = out_toLoad.at(idx);
			auto pAsset = Resources::inst().create<T>(std::move(asset.id), std::move(asset.info));
			if (pAsset)
			{
				std::scoped_lock<decltype(mutex)> lock(mutex);
				out_assets.push_back(pAsset);
				out_loaded.push_back(asset.id);
			}
		};
		task.name = jobName;
		task.bSilent = false;
		task.iterationCount = out_toLoad.size();
		return task;
	}
	return {};
}
} // namespace

void AssetManifest::Info::intersect(AssetList ids)
{
	intersectTlist<Shader>(shaders, ids.shaders);
	intersectTlist<Texture>(textures, ids.textures);
	intersectTlist<Texture>(cubemaps, ids.cubemaps);
	intersectTlist<Material>(materials, ids.materials);
	intersectTlist<Mesh>(meshes, ids.meshes);
	intersectTlist<Model>(models, ids.models);
	intersectTlist<Font>(fonts, ids.fonts);
}

AssetList AssetManifest::Info::exportList() const
{
	AssetList ret;
	ret.shaders = exportTList(shaders);
	ret.textures = exportTList(textures);
	ret.cubemaps = exportTList(cubemaps);
	ret.materials = exportTList(materials);
	ret.meshes = exportTList(meshes);
	ret.models = exportTList(models);
	ret.fonts = exportTList(fonts);
	return ret;
}

bool AssetManifest::Info::isEmpty() const
{
	return empty(shaders, textures, cubemaps, materials, meshes, models, fonts);
}

std::string AssetList::print() const
{
	std::stringstream ret;
	auto add = [&ret](std::vector<stdfs::path> const& vec, std::string_view title) {
		ret << title << "\n";
		for (auto const& id : vec)
		{
			ret << "\t" << id.generic_string() << "\n";
		}
	};
	add(shaders, "Shaders");
	add(textures, "Textures");
	add(cubemaps, "Cubemaps");
	add(materials, "Materials");
	add(meshes, "Meshes");
	add(models, "Models");
	add(fonts, "Fonts");
	return ret.str();
}

std::string const AssetManifest::s_tName = utils::tName<AssetManifest>();

AssetManifest::AssetManifest(IOReader const& reader, stdfs::path const& id) : m_pReader(&reader)
{
	ASSERT(m_pReader, "IOReader is null!");
	auto [data, bResult] = m_pReader->getString(id);
	if (bResult)
	{
		m_manifest = GData(std::move(data));
	}
	else
	{
		ASSERT(false, "Manifest not found!");
		LOG_E("[{}] Manifest [{}] not found on [{}]!", s_tName, id.generic_string(), m_pReader->medium());
	}
}

AssetManifest::~AssetManifest()
{
	while (update(true) != Status::eIdle)
	{
		engine::update();
		threads::sleep();
	}
}

void AssetManifest::start()
{
	if (!m_bParsed)
	{
		parse();
	}
	if (!m_toLoad.isEmpty())
	{
		loadData();
	}
}

AssetManifest::Status AssetManifest::update(bool bTerminate)
{
	switch (m_status)
	{
	default:
	{
		if (bTerminate)
		{
			m_status = Status::eIdle;
			m_loaded = {};
		}
		break;
	}
	case Status::eExtractingData:
	{
		if (eraseDone(bTerminate))
		{
			if (bTerminate)
			{
				m_status = Status::eIdle;
				m_loaded = {};
			}
			else
			{
				loadAssets();
			}
		}
		break;
	}
	case Status::eLoadingAssets:
	{
		if (eraseDone(bTerminate))
		{
			m_toLoad = {};
			m_loading.clear();
			m_semaphore.reset();
			m_status = Status::eIdle;
			if (bTerminate)
			{
				m_loaded = {};
			}
		}
	}
	}
	bool const bTerminating = bTerminate && m_status == Status::eLoadingAssets;
	return bTerminating ? Status::eTerminating : m_status;
}

AssetList AssetManifest::parse()
{
	AssetList all;
	auto const shaders = m_manifest.getGDatas("shaders");
	for (auto const& shader : shaders)
	{
		auto assetID = shader.getString("id");
		if (!assetID.empty())
		{
			if (Resources::inst().get<Shader>(assetID))
			{
				all.shaders.push_back(assetID);
				m_loaded.shaders.push_back(std::move(assetID));
			}
			else
			{
				AssetData<Shader> data;
				data.info.pReader = m_pReader;
				data.id = std::move(assetID);
				bool const bOptional = shader.getBool("optional", false);
				bool bFound = false;
				auto const resourceIDs = shader.getGDatas("resource_ids");
				for (auto const& resourceID : resourceIDs)
				{
					auto const typeStr = resourceID.getString("type");
					auto const id = resourceID.getString("id");
					bool const bPresent = bOptional ? m_pReader->checkPresence(id) : m_pReader->isPresent(id);
					if (!id.empty() && !typeStr.empty() && bPresent)
					{
						Shader::Type type = Shader::Type::eFragment;
						if (typeStr == "vertex")
						{
							type = Shader::Type::eVertex;
						}
						data.info.codeIDMap[(size_t)type] = id;
						bFound = true;
					}
				}
				if (bFound)
				{
					all.shaders.push_back(data.id);
					m_toLoad.shaders.push_back(std::move(data));
					m_data.idCount.fetch_add(1);
				}
			}
		}
	}
	auto const textures = m_manifest.getGDatas("textures");
	for (auto const& texture : textures)
	{
		auto const id = texture.getString("id");
		auto const colourSpace = texture.getString("colour_space");
		bool const bPresent = texture.getBool("optional", false) ? m_pReader->isPresent(id) : m_pReader->checkPresence(id);
		if (!id.empty() && bPresent)
		{
			if (Resources::inst().get<Texture>(id))
			{
				all.textures.push_back(id);
				m_loaded.textures.push_back(std::move(id));
			}
			else
			{
				AssetData<Texture> data;
				data.id = id;
				data.info.mode = engine::colourSpace();
				data.info.samplerID = m_manifest.getString("sampler");
				data.info.pReader = m_pReader;
				data.info.ids = {id};
				all.textures.push_back(id);
				m_toLoad.textures.push_back(std::move(data));
				m_data.idCount.fetch_add(1);
			}
		}
	}
	auto const cubemaps = m_manifest.getGDatas("cubemaps");
	for (auto const& cubemap : cubemaps)
	{
		auto const assetID = cubemap.getString("id");
		auto const colourSpace = cubemap.getString("colour_space");
		auto const resourceIDs = cubemap.getVecString("textures");
		bool const bOptional = cubemap.getBool("optional", false);
		bool bMissing = false;
		if (!assetID.empty())
		{
			if (Resources::inst().get<Texture>(assetID))
			{
				all.cubemaps.push_back(assetID);
				m_loaded.cubemaps.push_back(std::move(assetID));
			}
			else
			{
				AssetData<Texture> data;
				data.id = assetID;
				data.info.mode = engine::colourSpace();
				data.info.type = Texture::Type::eCube;
				data.info.samplerID = m_manifest.getString("sampler");
				data.info.pReader = m_pReader;
				for (auto const& id : resourceIDs)
				{
					bool const bPresent = bOptional ? m_pReader->isPresent(id) : m_pReader->checkPresence(id);
					if (bPresent)
					{
						data.info.ids.push_back(id);
					}
					else
					{
						bMissing = true;
						break;
					}
				}
				if (!bMissing)
				{
					all.cubemaps.push_back(data.id);
					m_toLoad.cubemaps.push_back(std::move(data));
					m_data.idCount.fetch_add(1);
				}
			}
		}
	}
	auto const models = m_manifest.getGDatas("models");
	for (auto const& model : models)
	{
		auto const modelID = model.getString("id");
		if (!modelID.empty())
		{
			if (Resources::inst().get<Model>(modelID))
			{
				all.models.push_back(modelID);
				m_loaded.models.push_back(std::move(modelID));
			}
			else
			{
				bool const bOptional = model.getBool("optional", false);
				AssetData<Model> data;
				data.id = modelID;
				auto jsonID = data.id / data.id.filename();
				jsonID += ".json";
				bool const bPresent = bOptional ? m_pReader->isPresent(jsonID) : m_pReader->checkPresence(jsonID);
				if (bPresent)
				{
					all.models.push_back(data.id);
					m_toLoad.models.push_back(std::move(data));
					m_data.idCount.fetch_add(1);
				}
			}
		}
	}
	m_bParsed = true;
	return all;
}

void AssetManifest::unload(const AssetList& list)
{
	auto unload = [](std::vector<stdfs::path> const& ids) {
		for (auto const& id : ids)
		{
			Resources::inst().unload(id);
		}
	};
	unload(list.shaders);
	unload(list.textures);
	unload(list.cubemaps);
	unload(list.materials);
	unload(list.meshes);
	unload(list.models);
	unload(list.fonts);
}

void AssetManifest::loadData()
{
	m_status = Status::eExtractingData;
	IndexedTask task;
	if (!m_toLoad.models.empty())
	{
		task.task = [this](size_t idx) {
			auto& model = m_toLoad.models.at(idx);
			gfx::Model::LoadRequest mlr;
			mlr.assetID = model.id;
			mlr.pReader = m_pReader;
			model.info = gfx::Model::parseOBJ(mlr);
		};
		task.name = "Manifest-0:Models";
		task.iterationCount = m_toLoad.models.size();
		addJobs(std::move(task));
	}
}

void AssetManifest::loadAssets()
{
	m_status = Status::eLoadingAssets;
	m_semaphore = Resources::inst().setBusy();
	IndexedTask task;
	addJobs(loadTAssets(m_toLoad.shaders, m_loaded.shaders, m_loading, m_mutex, "Manifest-1:Shaders"));
	addJobs(loadTAssets(m_toLoad.textures, m_loaded.textures, m_loading, m_mutex, "Manifest-1:Textures"));
	addJobs(loadTAssets(m_toLoad.cubemaps, m_loaded.cubemaps, m_loading, m_mutex, "Manifest-1:Cubemaps"));
	addJobs(loadTAssets(m_toLoad.models, m_loaded.models, m_loading, m_mutex, "Manifest-1:Models"));
}

bool AssetManifest::eraseDone(bool bWaitingJobs)
{
	std::scoped_lock<decltype(m_mutex)> lock(m_mutex);
	if (!m_running.empty())
	{
		auto iter = std::remove_if(m_running.begin(), m_running.end(), [bWaitingJobs](auto const& sJob) -> bool {
			bool bRet = sJob->hasCompleted();
			if (bWaitingJobs)
			{
				bRet |= sJob->discard();
			}
			return bRet;
		});
		m_running.erase(iter, m_running.end());
	}
	if (!m_loading.empty())
	{
		auto iter = std::remove_if(m_loading.begin(), m_loading.end(), [](Asset* pAsset) { return !pAsset->isBusy(); });
		m_loading.erase(iter, m_loading.end());
	}
	return m_running.empty() && m_loading.empty();
}

void AssetManifest::addJobs(IndexedTask task)
{
	auto hjobs = jobs::forEach(std::move(task));
	std::move(hjobs.begin(), hjobs.end(), std::back_inserter(m_running));
}

void AssetManifest::addJobs(TResult<IndexedTask> task)
{
	if (task.bResult)
	{
		auto hjobs = jobs::forEach(task.payload);
		std::move(hjobs.begin(), hjobs.end(), std::back_inserter(m_running));
	}
}
} // namespace le
