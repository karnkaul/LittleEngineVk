#include <algorithm>
#include <sstream>
#include <utility>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/threads.hpp>
#include <core/utils.hpp>
#include <engine/resources/resources.hpp>
#include <assets/manifest.hpp>
#include <resources/resources_impl.hpp>
#include <levk_impl.hpp>

namespace le
{
using namespace gfx;
using namespace res;

namespace
{
template <typename T>
void intersectTlist(std::vector<T>& out_data, std::vector<stdfs::path> const& src)
{
	auto iter = std::remove_if(out_data.begin(), out_data.end(), [&src](auto const& data) {
		return std::find_if(src.begin(), src.end(), [&data](auto const& id) { return data.id == id; }) == src.end();
	});
	out_data.erase(iter, out_data.end());
}

template <typename T>
std::vector<stdfs::path> exportTList(std::vector<T> const& src)
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
std::vector<std::shared_ptr<tasks::Handle>> loadTAssets(std::vector<AssetData<T>>& out_toLoad, std::vector<stdfs::path>& out_loaded,
														std::vector<Asset*>& out_assets, Lockable<std::mutex>& mutex, std::string_view jobName)
{
	static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
	tasks::List taskList;
	if (!out_toLoad.empty())
	{
		auto task = [&out_loaded, &out_assets, &mutex](AssetData<T>& data) {
			auto pAsset = Resources::inst().create<T>(data.id, std::move(data.info));
			if (pAsset)
			{
				auto lock = mutex.lock();
				out_assets.push_back(pAsset);
				out_loaded.push_back(std::move(data.id));
			}
		};
		return tasks::forEach<AssetData<T>>(out_toLoad, task, jobName);
	}
	return {};
}

template <typename T>
std::vector<std::shared_ptr<tasks::Handle>> loadTResources(std::vector<ResourceData<T>>& out_toLoad, std::vector<stdfs::path>& out_loaded,
														   std::vector<res::GUID>& out_assets, Lockable<std::mutex>& mutex, std::string_view jobName)
{
	static_assert(std::is_base_of_v<Resource, T>, "T must derive from Asset!");
	tasks::List taskList;
	if (!out_toLoad.empty())
	{
		auto task = [&out_loaded, &out_assets, &mutex](ResourceData<T>& data) {
			auto resource = res::load<T>(data.id, std::move(data.createInfo));
			if (resource.guid > GUID::s_null)
			{
				auto lock = mutex.lock();
				out_assets.push_back(resource.guid);
				out_loaded.push_back(std::move(data.id));
			}
		};
		return tasks::forEach<ResourceData<T>>(out_toLoad, task, jobName);
	}
	return {};
}
} // namespace

void AssetManifest::Info::intersect(AssetList ids)
{
	intersectTlist(shaders, ids.shaders);
	intersectTlist(textures, ids.textures);
	intersectTlist(cubemaps, ids.cubemaps);
	intersectTlist(materials, ids.materials);
	intersectTlist(meshes, ids.meshes);
	intersectTlist(models, ids.models);
	intersectTlist(fonts, ids.fonts);
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

AssetManifest::AssetManifest(io::Reader const& reader, stdfs::path const& id) : m_pReader(&reader)
{
	ASSERT(m_pReader, "io::Reader is null!");
	auto [data, bResult] = m_pReader->getString(id);
	if (bResult)
	{
		if (!m_manifest.read(std::move(data)))
		{
			LOG_E("[{}] Failed to read manifest [{}] from [{}]", s_tName, id.generic_string(), m_pReader->medium());
			m_manifest.clear();
		}
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
	auto const shaders = m_manifest.get<std::vector<GData>>("shaders");
	for (auto const& shader : shaders)
	{
		auto assetID = shader.get("id");
		if (!assetID.empty())
		{
			if (res::find<Shader>(assetID).bResult)
			{
				all.shaders.push_back(assetID);
				m_loaded.shaders.push_back(std::move(assetID));
			}
			else
			{
				ResourceData<Shader> data;
				data.id = std::move(assetID);
				bool const bOptional = shader.get<bool>("optional");
				bool bFound = false;
				auto const resourceIDs = shader.get<std::vector<GData>>("resource_ids");
				for (auto const& resourceID : resourceIDs)
				{
					auto const typeStr = resourceID.get("type");
					auto const id = resourceID.get("id");
					bool const bPresent = bOptional ? m_pReader->checkPresence(id) : m_pReader->isPresent(id);
					if (!id.empty() && !typeStr.empty() && bPresent)
					{
						Shader::Type type = Shader::Type::eFragment;
						if (typeStr == "vertex")
						{
							type = Shader::Type::eVertex;
						}
						data.createInfo.codeIDMap[(std::size_t)type] = id;
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
	auto const textures = m_manifest.get<std::vector<GData>>("textures");
	for (auto const& texture : textures)
	{
		auto const id = texture.get("id");
		bool const bPresent = texture.get<bool>("optional") ? m_pReader->isPresent(id) : m_pReader->checkPresence(id);
		if (!id.empty() && bPresent)
		{
			if (res::findTexture(id).bResult)
			{
				all.textures.push_back(id);
				m_loaded.textures.push_back(std::move(id));
			}
			else
			{
				ResourceData<res::Texture> data;
				data.id = id;
				data.createInfo.mode = engine::colourSpace();
				data.createInfo.samplerID = m_manifest.get("sampler");
				data.createInfo.ids = {id};
				all.textures.push_back(id);
				m_toLoad.textures.push_back(std::move(data));
				m_data.idCount.fetch_add(1);
			}
		}
	}
	auto const cubemaps = m_manifest.get<std::vector<GData>>("cubemaps");
	for (auto const& cubemap : cubemaps)
	{
		auto const assetID = cubemap.get("id");
		auto const resourceIDs = cubemap.get<std::vector<std::string>>("textures");
		bool const bOptional = cubemap.get<bool>("optional");
		bool bMissing = false;
		if (!assetID.empty())
		{
			if (res::findTexture(assetID).bResult)
			{
				all.cubemaps.push_back(assetID);
				m_loaded.cubemaps.push_back(std::move(assetID));
			}
			else
			{
				ResourceData<res::Texture> data;
				data.id = assetID;
				data.createInfo.mode = engine::colourSpace();
				data.createInfo.type = res::Texture::Type::eCube;
				data.createInfo.samplerID = m_manifest.get("sampler");
				for (auto const& id : resourceIDs)
				{
					bool const bPresent = bOptional ? m_pReader->isPresent(id) : m_pReader->checkPresence(id);
					if (bPresent)
					{
						data.createInfo.ids.push_back(id);
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
	auto const models = m_manifest.get<std::vector<GData>>("models");
	for (auto const& model : models)
	{
		auto const modelID = model.get("id");
		if (!modelID.empty())
		{
			if (Resources::inst().get<Model>(modelID))
			{
				all.models.push_back(modelID);
				m_loaded.models.push_back(std::move(modelID));
			}
			else
			{
				bool const bOptional = model.get<bool>("optional");
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
	auto unload2 = [](std::vector<stdfs::path> const& ids) {
		for (auto const& id : ids)
		{
			res::unload(id);
		}
	};
	unload2(list.shaders);
	unload2(list.textures);
	unload2(list.cubemaps);
	unload2(list.materials);
	unload(list.meshes);
	unload(list.models);
	unload(list.fonts);
}

void AssetManifest::loadData()
{
	m_status = Status::eExtractingData;
	if (!m_toLoad.models.empty())
	{
		auto task = [](AssetData<Model>& data) { data.info = gfx::Model::parseOBJ(data.id); };
		addJobs(tasks::forEach<AssetData<Model>>(m_toLoad.models, task, "Manifest-0:Models"));
	}
}

void AssetManifest::loadAssets()
{
	m_status = Status::eLoadingAssets;
	m_semaphore = Resources::inst().setBusy();
	addJobs(loadTResources(m_toLoad.shaders, m_loaded.shaders, m_loading2, m_mutex, "Manifest-1:Shaders"));
	addJobs(loadTResources(m_toLoad.textures, m_loaded.textures, m_loading2, m_mutex, "Manifest-1:Textures"));
	addJobs(loadTResources(m_toLoad.cubemaps, m_loaded.cubemaps, m_loading2, m_mutex, "Manifest-1:Cubemaps"));
	addJobs(loadTAssets(m_toLoad.models, m_loaded.models, m_loading, m_mutex, "Manifest-1:Models"));
}

bool AssetManifest::eraseDone(bool bWaitingJobs)
{
	auto lock = m_mutex.lock();
	if (!m_running.empty())
	{
		auto iter = std::remove_if(m_running.begin(), m_running.end(), [bWaitingJobs](auto const& sJob) -> bool {
			bool bRet = sJob->hasCompleted(true);
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
	if (!m_loading2.empty())
	{
		auto iter = std::remove_if(m_loading2.begin(), m_loading2.end(), [](res::GUID guid) { return !res::isLoading(guid); });
		m_loading2.erase(iter, m_loading2.end());
	}
	return m_running.empty() && m_loading.empty() && m_loading2.empty();
}

void AssetManifest::addJobs(std::vector<std::shared_ptr<tasks::Handle>> handles)
{
	std::move(handles.begin(), handles.end(), std::back_inserter(m_running));
}
} // namespace le
