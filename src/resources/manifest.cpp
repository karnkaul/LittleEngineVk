#include <algorithm>
#include <sstream>
#include <utility>
#include <core/ensure.hpp>
#include <core/log.hpp>
#include <core/threads.hpp>
#include <core/utils.hpp>
#include <engine/levk.hpp>
#include <engine/resources/resources.hpp>
#include <levk_impl.hpp>
#include <resources/manifest.hpp>
#include <resources/resources_impl.hpp>

namespace le::res {
namespace {
template <typename T>
void intersectTlist(std::vector<T>& out_data, std::vector<io::Path> const& src) {
	auto iter = std::remove_if(out_data.begin(), out_data.end(), [&src](auto const& data) {
		return std::find_if(src.begin(), src.end(), [&data](auto const& id) { return data.id == id; }) == src.end();
	});
	out_data.erase(iter, out_data.end());
}

template <typename T>
std::vector<io::Path> exportTList(std::vector<T> const& src) {
	std::vector<io::Path> ret;
	for (auto const& data : src) {
		ret.push_back(data.id);
	}
	return ret;
}

template <typename... T>
bool allEmpty(std::vector<T> const&... out_vecs) {
	return (out_vecs.empty() && ...);
}

template <typename T>
std::vector<std::shared_ptr<tasks::Handle>> loadTResources(std::vector<ResourceData<T>>& out_toLoad, std::vector<io::Path>& out_loaded,
														   std::vector<GUID>& out_resources, kt::lockable<std::mutex>& mutex, std::string_view jobName) {
	static_assert(std::is_base_of_v<Resource<T>, T>, "T must derive from Resource!");
	if (!out_toLoad.empty()) {
		auto task = [&out_loaded, &out_resources, &mutex](ResourceData<T>& data) {
			auto resource = load(data.id, std::move(data.createInfo));
			if (resource.guid > GUID::null) {
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

void Manifest::Info::intersect(ResourceList ids) {
	intersectTlist(shaders, ids.shaders);
	intersectTlist(textures, ids.textures);
	intersectTlist(cubemaps, ids.cubemaps);
	intersectTlist(materials, ids.materials);
	intersectTlist(meshes, ids.meshes);
	intersectTlist(models, ids.models);
	intersectTlist(fonts, ids.fonts);
}

ResourceList Manifest::Info::exportList() const {
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

bool Manifest::Info::empty() const {
	return allEmpty(shaders, textures, cubemaps, materials, meshes, models, fonts);
}

std::string const Manifest::s_tName = utils::tName<Manifest>();

bool Manifest::read(io::Path const& id) {
	if (auto data = engine::reader().string(id)) {
		auto node = dj::node_t::make(*data);
		if (!node) {
			logE("[{}] Failed to read manifest [{}] from [{}]", s_tName, id.generic_string(), engine::reader().medium());
			m_manifest = {};
			return false;
		}
		m_manifest = std::move(*node);
		m_status = Status::eReady;
		return true;
	} else {
		ENSURE(false, "Manifest not found!");
		logE("[{}] Manifest [{}] not found on [{}]!", s_tName, id.generic_string(), engine::reader().medium());
	}
	return false;
}

void Manifest::start() {
	if (!m_bParsed) {
		parse();
	}
	if (!m_toLoad.empty()) {
		loadData();
	}
	m_status = Status::eExtractingData;
}

Manifest::Status Manifest::update(bool bTerminate) {
	switch (m_status) {
	default: {
		if (bTerminate) {
			m_status = Status::eIdle;
			m_loaded = {};
		}
		break;
	}
	case Status::eExtractingData: {
		if (eraseDone(bTerminate)) {
			if (bTerminate) {
				m_status = Status::eIdle;
				m_loaded = {};
			} else {
				loadResources();
			}
		}
		break;
	}
	case Status::eLoadingResources: {
		if (eraseDone(bTerminate)) {
			m_toLoad = {};
			m_loading.clear();
			m_semaphore.reset();
			m_status = Status::eIdle;
			if (bTerminate) {
				m_loaded = {};
			}
		}
	}
	}
	bool const bTerminating = bTerminate && m_status == Status::eLoadingResources;
	return bTerminating ? Status::eTerminating : m_status;
}

ResourceList Manifest::parse() {
	ResourceList all;
	if (auto pShaders = m_manifest.find("shaders")) {
		for (auto const& shader : pShaders->as<dj::array_nodes_t>()) {
			auto resourceID = shader->safe_get("id").as<std::string>();
			if (!resourceID.empty()) {
				if (find<Shader>(resourceID)) {
					all.shaders.push_back(resourceID);
					m_loaded.shaders.push_back(std::move(resourceID));
				} else {
					ResourceData<Shader> data;
					data.id = std::move(resourceID);
					bool const bOptional = shader->safe_get("optional").as<bool>();
					bool bFound = false;
					if (auto const pResourceIDs = shader->find("resource_ids")) {
						for (auto const& resourceID : pResourceIDs->as<dj::array_nodes_t>()) {
							auto const typeStr = resourceID->safe_get("type").as<std::string>();
							auto const id = resourceID->safe_get("id").as<std::string>();
							bool const bPresent = bOptional ? engine::reader().checkPresence(id) : engine::reader().isPresent(id);
							if (!id.empty() && !typeStr.empty() && bPresent) {
								Shader::Type type = Shader::Type::eFragment;
								if (typeStr == "vertex") {
									type = Shader::Type::eVertex;
								}
								data.createInfo.codeIDMap[(std::size_t)type] = id;
								bFound = true;
							}
						}
						if (bFound) {
							all.shaders.push_back(data.id);
							m_toLoad.shaders.push_back(std::move(data));
							m_data.idCount.fetch_add(1);
						}
					}
				}
			}
		}
	}
	if (auto pTextures = m_manifest.find("textures")) {
		for (auto const& texture : pTextures->as<dj::array_nodes_t>()) {
			auto const id = texture->safe_get("id").as<std::string>();
			bool const bPresent = texture->safe_get("optional").as<bool>() ? engine::reader().isPresent(id) : engine::reader().checkPresence(id);
			if (!id.empty() && bPresent) {
				if (find<Texture>(id)) {
					all.textures.push_back(id);
					m_loaded.textures.push_back(std::move(id));
				} else {
					ResourceData<Texture> data;
					data.id = id;
					data.createInfo.mode = engine::colourSpace();
					data.createInfo.samplerID = texture->safe_get("sampler").as<std::string>();
					data.createInfo.ids = {id};
					all.textures.push_back(id);
					m_toLoad.textures.push_back(std::move(data));
					m_data.idCount.fetch_add(1);
				}
			}
		}
	}
	if (auto pCubemaps = m_manifest.find("cubemaps")) {
		for (auto const& cubemap : pCubemaps->as<dj::array_nodes_t>()) {
			auto const resourceID = cubemap->safe_get("id").as<std::string>();
			bool const bOptional = cubemap->safe_get("optional").as<bool>();
			bool bMissing = false;
			auto const pResourceIDs = cubemap->find("textures");
			if (!resourceID.empty()) {
				if (find<Texture>(resourceID)) {
					all.cubemaps.push_back(resourceID);
					m_loaded.cubemaps.push_back(std::move(resourceID));
				} else if (pResourceIDs) {
					ResourceData<Texture> data;
					data.id = resourceID;
					data.createInfo.mode = engine::colourSpace();
					data.createInfo.type = Texture::Type::eCube;
					data.createInfo.samplerID = cubemap->safe_get("sampler").as<std::string>();
					for (auto const& id : pResourceIDs->as<dj::array_fields_t<std::string>>()) {
						bool const bPresent = bOptional ? engine::reader().isPresent(id) : engine::reader().checkPresence(id);
						if (bPresent) {
							data.createInfo.ids.push_back(id);
						} else {
							bMissing = true;
						}
					}
					if (!bMissing) {
						all.cubemaps.push_back(data.id);
						m_toLoad.cubemaps.push_back(std::move(data));
						m_data.idCount.fetch_add(1);
					}
				}
			}
		}
	}
	if (auto pModels = m_manifest.find("models")) {
		for (auto const& model : pModels->as<dj::array_nodes_t>()) {
			auto const modelID = model->safe_get("id").as<std::string>();
			if (!modelID.empty()) {
				if (find<Model>(modelID)) {
					all.models.push_back(modelID);
					m_loaded.models.push_back(std::move(modelID));
				} else {
					bool const bOptional = model->safe_get("optional").as<bool>();
					ResourceData<Model> data;
					data.id = modelID;
					auto jsonID = data.id / data.id.filename();
					jsonID += ".json";
					bool const bPresent = bOptional ? engine::reader().isPresent(jsonID) : engine::reader().checkPresence(jsonID);
					if (bPresent) {
						all.models.push_back(data.id);
						m_toLoad.models.push_back(std::move(data));
						m_data.idCount.fetch_add(1);
					}
				}
			}
		}
	}
	m_bParsed = true;
	return all;
}

void Manifest::reset() {
	m_loaded = {};
	m_toLoad = {};
	m_manifest = {};
	m_data.idCount = 0;
	m_data.dataCount = 0;
	m_running.clear();
	m_loading.clear();
	m_semaphore.reset();
	m_status = Status::eIdle;
	m_bParsed = false;
}

bool Manifest::idle() const {
	return m_status == Status::eIdle;
}

bool Manifest::ready() const {
	return m_status == Status::eReady;
}

void Manifest::unload(const ResourceList& list) {
	auto unload = [](std::vector<io::Path> const& ids) {
		for (auto const& id : ids) {
			res::unload(Hash(id));
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

void Manifest::loadData() {
	m_status = Status::eExtractingData;
	if (!m_toLoad.models.empty()) {
		auto task = [](ResourceData<Model>& data) {
			Model::LoadInfo loadInfo;
			loadInfo.idRoot = data.id;
			loadInfo.jsonDirectory = data.id;
			auto info = loadInfo.createInfo();
			if (info) {
				data.createInfo = std::move(*info);
			}
		};
		addJobs(tasks::forEach<ResourceData<Model>>(m_toLoad.models, task, "Manifest-0:Models"));
	}
}

void Manifest::loadResources() {
	m_status = Status::eLoadingResources;
	m_semaphore = acquire();
	addJobs(loadTResources(m_toLoad.shaders, m_loaded.shaders, m_loading, m_mutex, "Manifest-1:Shaders"));
	addJobs(loadTResources(m_toLoad.textures, m_loaded.textures, m_loading, m_mutex, "Manifest-1:Textures"));
	addJobs(loadTResources(m_toLoad.cubemaps, m_loaded.cubemaps, m_loading, m_mutex, "Manifest-1:Cubemaps"));
	addJobs(loadTResources(m_toLoad.models, m_loaded.models, m_loading, m_mutex, "Manifest-1:Models"));
}

bool Manifest::eraseDone(bool bWaitingJobs) {
	auto lock = m_mutex.lock();
	if (!m_running.empty()) {
		auto iter = std::remove_if(m_running.begin(), m_running.end(), [bWaitingJobs](auto const& sJob) -> bool {
			bool bRet = sJob->hasCompleted(true);
			if (bWaitingJobs) {
				bRet |= sJob->discard();
			}
			return bRet;
		});
		m_running.erase(iter, m_running.end());
	}
	if (!m_loading.empty()) {
		auto iter = std::remove_if(m_loading.begin(), m_loading.end(), [](GUID guid) { return !isLoading(guid); });
		m_loading.erase(iter, m_loading.end());
	}
	return m_running.empty() && m_loading.empty() && m_loading.empty();
}

void Manifest::addJobs(std::vector<std::shared_ptr<tasks::Handle>> handles) {
	std::move(handles.begin(), handles.end(), std::back_inserter(m_running));
}
} // namespace le::res
