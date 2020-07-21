#include <algorithm>
#include <sstream>
#include <utility>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/threads.hpp>
#include <core/utils.hpp>
#include <engine/resources/resources.hpp>
#include <resources/manifest.hpp>
#include <resources/resources_impl.hpp>
#include <levk_impl.hpp>

namespace le::res
{
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
std::vector<std::shared_ptr<tasks::Handle>> loadTResources(std::vector<ResourceData<T>>& out_toLoad, std::vector<stdfs::path>& out_loaded,
														   std::vector<GUID>& out_resources, Lockable<std::mutex>& mutex, std::string_view jobName)
{
	static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource!");
	tasks::List taskList;
	if (!out_toLoad.empty())
	{
		auto task = [&out_loaded, &out_resources, &mutex](ResourceData<T>& data) {
			auto resource = load<T>(data.id, std::move(data.createInfo));
			if (resource.guid > GUID::s_null)
			{
				auto lock = mutex.lock();
				out_resources.push_back(resource.guid);
				out_loaded.push_back(std::move(data.id));
			}
		};
		return tasks::forEach<ResourceData<T>>(out_toLoad, task, jobName);
	}
	return {};
}
} // namespace

void Manifest::Info::intersect(ResourceList ids)
{
	intersectTlist(shaders, ids.shaders);
	intersectTlist(textures, ids.textures);
	intersectTlist(cubemaps, ids.cubemaps);
	intersectTlist(materials, ids.materials);
	intersectTlist(meshes, ids.meshes);
	intersectTlist(models, ids.models);
	intersectTlist(fonts, ids.fonts);
}

ResourceList Manifest::Info::exportList() const
{
	ResourceList ret;
	ret.shaders = exportTList(shaders);
	ret.textures = exportTList(textures);
	ret.cubemaps = exportTList(cubemaps);
	ret.materials = exportTList(materials);
	ret.meshes = exportTList(meshes);
	ret.models = exportTList(models);
	ret.fonts = exportTList(fonts);
	return ret;
}

bool Manifest::Info::isEmpty() const
{
	return empty(shaders, textures, cubemaps, materials, meshes, models, fonts);
}

std::string ResourceList::print() const
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

std::string const Manifest::s_tName = utils::tName<Manifest>();

Manifest::Manifest(io::Reader const& reader, stdfs::path const& id) : m_pReader(&reader)
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

Manifest::~Manifest()
{
	while (update(true) != Status::eIdle)
	{
		engine::update();
		threads::sleep();
	}
}

void Manifest::start()
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

Manifest::Status Manifest::update(bool bTerminate)
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
				loadResources();
			}
		}
		break;
	}
	case Status::eLoadingResources:
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
	bool const bTerminating = bTerminate && m_status == Status::eLoadingResources;
	return bTerminating ? Status::eTerminating : m_status;
}

ResourceList Manifest::parse()
{
	ResourceList all;
	auto const shaders = m_manifest.get<std::vector<GData>>("shaders");
	for (auto const& shader : shaders)
	{
		auto resourceID = shader.get("id");
		if (!resourceID.empty())
		{
			if (find<Shader>(resourceID).bResult)
			{
				all.shaders.push_back(resourceID);
				m_loaded.shaders.push_back(std::move(resourceID));
			}
			else
			{
				ResourceData<Shader> data;
				data.id = std::move(resourceID);
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
			if (findTexture(id).bResult)
			{
				all.textures.push_back(id);
				m_loaded.textures.push_back(std::move(id));
			}
			else
			{
				ResourceData<Texture> data;
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
		auto const resourceID = cubemap.get("id");
		auto const resourceIDs = cubemap.get<std::vector<std::string>>("textures");
		bool const bOptional = cubemap.get<bool>("optional");
		bool bMissing = false;
		if (!resourceID.empty())
		{
			if (findTexture(resourceID).bResult)
			{
				all.cubemaps.push_back(resourceID);
				m_loaded.cubemaps.push_back(std::move(resourceID));
			}
			else
			{
				ResourceData<Texture> data;
				data.id = resourceID;
				data.createInfo.mode = engine::colourSpace();
				data.createInfo.type = Texture::Type::eCube;
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
			if (findModel(modelID).bResult)
			{
				all.models.push_back(modelID);
				m_loaded.models.push_back(std::move(modelID));
			}
			else
			{
				bool const bOptional = model.get<bool>("optional");
				ResourceData<Model> data;
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

void Manifest::unload(const ResourceList& list)
{
	auto unload = [](std::vector<stdfs::path> const& ids) {
		for (auto const& id : ids)
		{
			res::unload(id);
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

void Manifest::loadData()
{
	m_status = Status::eExtractingData;
	if (!m_toLoad.models.empty())
	{
		auto task = [](ResourceData<Model>& data) { data.createInfo = Model::parseOBJ(data.id); };
		addJobs(tasks::forEach<ResourceData<Model>>(m_toLoad.models, task, "Manifest-0:Models"));
	}
}

void Manifest::loadResources()
{
	m_status = Status::eLoadingResources;
	m_semaphore = acquire();
	addJobs(loadTResources(m_toLoad.shaders, m_loaded.shaders, m_loading, m_mutex, "Manifest-1:Shaders"));
	addJobs(loadTResources(m_toLoad.textures, m_loaded.textures, m_loading, m_mutex, "Manifest-1:Textures"));
	addJobs(loadTResources(m_toLoad.cubemaps, m_loaded.cubemaps, m_loading, m_mutex, "Manifest-1:Cubemaps"));
	addJobs(loadTResources(m_toLoad.models, m_loaded.models, m_loading, m_mutex, "Manifest-1:Models"));
}

bool Manifest::eraseDone(bool bWaitingJobs)
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
		auto iter = std::remove_if(m_loading.begin(), m_loading.end(), [](GUID guid) { return !isLoading(guid); });
		m_loading.erase(iter, m_loading.end());
	}
	return m_running.empty() && m_loading.empty() && m_loading.empty();
}

void Manifest::addJobs(std::vector<std::shared_ptr<tasks::Handle>> handles)
{
	std::move(handles.begin(), handles.end(), std::back_inserter(m_running));
}
} // namespace le::res
