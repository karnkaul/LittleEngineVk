#include <algorithm>
#include <utility>
#include <core/assert.hpp>
#include <core/jobs.hpp>
#include <core/log.hpp>
#include <core/threads.hpp>
#include <core/utils.hpp>
#include <engine/assets/manifest.hpp>
#include <levk_impl.hpp>

namespace le
{
using namespace gfx;

namespace
{
template <typename T>
std::vector<AssetData<T>> intersect(std::vector<AssetData<T>> const& lhs, std::vector<AssetData<T>> const& rhs)
{
	std::vector<AssetData<T>> ret;
	for (auto& asset : lhs)
	{
		auto const search = std::find_if(rhs.begin(), rhs.end(), [&asset](AssetData<T> const& rhs) -> bool { return asset.id == rhs.id; });
		if (search != rhs.end())
		{
			ret.push_back(asset);
		}
	}
	return ret;
}

template <typename T>
void subtract(std::vector<AssetData<T>> const& lhs, std::vector<AssetData<T>>& out_rhs)
{
	for (auto& asset : lhs)
	{
		auto iter =
			std::remove_if(out_rhs.begin(), out_rhs.end(), [&asset](AssetData<T> const& rhs) -> bool { return asset.id == rhs.id; });
		out_rhs.erase(iter, out_rhs.end());
	}
}

template <typename... T>
bool empty(std::vector<T> const&... out_vecs)
{
	return (out_vecs.empty() && ...);
}
} // namespace

AssetManifest::Info AssetManifest::Info::operator*(const Info& rhs) const
{
	Info ret;
	ret.shaders = intersect(shaders, rhs.shaders);
	ret.textures = intersect(textures, rhs.textures);
	ret.cubemaps = intersect(cubemaps, rhs.cubemaps);
	ret.materials = intersect(materials, rhs.materials);
	ret.meshes = intersect(meshes, rhs.meshes);
	ret.models = intersect(models, rhs.models);
	ret.fonts = intersect(fonts, rhs.fonts);
	return ret;
}

AssetManifest::Info AssetManifest::Info::operator-(const Info& rhs) const
{
	Info ret = rhs;
	subtract(shaders, ret.shaders);
	subtract(textures, ret.textures);
	subtract(cubemaps, ret.cubemaps);
	subtract(materials, ret.materials);
	subtract(meshes, ret.meshes);
	subtract(models, ret.models);
	subtract(fonts, ret.fonts);
	return ret;
}

bool AssetManifest::Info::isEmpty() const
{
	return empty(shaders, textures, cubemaps, materials, meshes, models, fonts);
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
	extractIDs();
}

AssetManifest::Status AssetManifest::update(bool bTerminate)
{
	switch (m_status)
	{
	default:
	{
		break;
	}
	case Status::eIdle:
	{
		if (m_info.isEmpty() && !bTerminate)
		{
			extractIDs();
		}
		break;
	}
	case Status::eExtractingIDs:
	{
		if (eraseDone())
		{
			if (bTerminate)
			{
				m_status = Status::eIdle;
			}
			else
			{
				m_cache = m_info;
				extractData();
			}
		}
		break;
	}
	case Status::eExtractingData:
	{
		if (eraseDone())
		{
			if (bTerminate)
			{
				m_status = Status::eIdle;
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
		if (eraseDone())
		{
			m_info = {};
			m_loading.clear();
			m_semaphore.reset();
			m_status = Status::eIdle;
		}
	}
	}
	bool const bTerminating = bTerminate && m_status == Status::eLoadingAssets;
	return bTerminating ? Status::eTerminating : m_status;
}

void AssetManifest::extractIDs()
{
	m_status = Status::eExtractingIDs;
	m_running.push_back(jobs::enqueue(
		[this]() {
			auto const shaders = m_manifest.getGDatas("shaders");
			for (auto const& shader : shaders)
			{
				auto assetID = shader.getString("id");
				if (!assetID.empty())
				{
					AssetData<Shader> data;
					data.info.pReader = m_pReader;
					data.id = std::move(assetID);
					auto const resourceIDs = shader.getGDatas("resource_ids");
					for (auto const& resourceID : resourceIDs)
					{
						auto const typeStr = resourceID.getString("type");
						auto const id = resourceID.getString("id");
						if (!id.empty() && !typeStr.empty() && m_pReader->checkPresence(id))
						{
							Shader::Type type = Shader::Type::eFragment;
							if (typeStr == "vertex")
							{
								type = Shader::Type::eVertex;
							}
							data.info.codeIDMap[(size_t)type] = id;
							m_info.shaders.push_back(std::move(data));
							m_data.idCount.fetch_add(1);
						}
					}
				}
			}
		},
		"", true));
	m_running.push_back(jobs::enqueue(
		[this]() {
			auto const textures = m_manifest.getGDatas("textures");
			for (auto const& texture : textures)
			{
				auto const id = texture.getString("id");
				auto const colourSpace = texture.getString("colour_space");
				if (!id.empty() && m_pReader->checkPresence(id))
				{
					AssetData<Texture> data;
					data.id = id;
					data.info.mode = engine::colourSpace();
					data.info.samplerID = m_manifest.getString("sampler");
					data.info.pReader = m_pReader;
					m_info.textures.push_back(std::move(data));
					m_data.idCount.fetch_add(1);
				}
			}
		},
		"", true));
	m_running.push_back(jobs::enqueue(
		[this]() {
			auto const cubemaps = m_manifest.getGDatas("cubemaps");
			for (auto const& cubemap : cubemaps)
			{
				auto const id = cubemap.getString("id");
				auto const colourSpace = cubemap.getString("colour_space");
				auto const resourceIDs = cubemap.getVecString("textures");
				if (!id.empty())
				{
					AssetData<Texture> data;
					data.id = id;
					data.info.mode = engine::colourSpace();
					data.info.type = Texture::Type::eCube;
					data.info.samplerID = m_manifest.getString("sampler");
					data.info.pReader = m_pReader;
					for (auto const& resourceID : resourceIDs)
					{
						data.info.ids.push_back(resourceID);
					}
					m_info.cubemaps.push_back(std::move(data));
					m_data.idCount.fetch_add(1);
				}
			}
		},
		"", true));
}

void AssetManifest::extractData()
{
	m_status = Status::eExtractingData;
	IndexedTask task;
	task.task = [this](size_t idx) {
		auto& texture = m_info.textures.at(idx);
		auto [bytes, bResult] = m_pReader->getBytes(texture.id);
		if (bResult)
		{
			texture.info.bytes = {std::move(bytes)};
		}
	};
	task.name = "TextureData";
	task.iterationCount = m_info.textures.size();
	addJobs(task);
	task.task = [this](size_t idx) {
		auto& cubemap = m_info.cubemaps.at(idx);
		for (auto id : cubemap.info.ids)
		{
			auto [bytes, bResult] = m_pReader->getBytes(id);
			if (bResult)
			{
				cubemap.info.bytes.push_back(std::move(bytes));
			}
		}
	};
	task.name = "CubemapData";
	task.iterationCount = m_info.cubemaps.size() - 1;
	addJobs(task);
}

void AssetManifest::loadAssets()
{
	m_status = Status::eLoadingAssets;
	m_semaphore = Resources::inst().setBusy();
	IndexedTask task;
	task.task = [this](size_t idx) {
		auto& texture = m_info.textures.at(idx);
		std::scoped_lock<decltype(m_mutex)> lock(m_mutex);
		m_loading.push_back(Resources::inst().create<Texture>(std::move(texture.id), std::move(texture.info)));
	};
	task.name = "Textures";
	task.iterationCount = m_info.textures.size();
	addJobs(task);
	task.task = [this](size_t idx) {
		auto& cubemap = m_info.cubemaps.at(idx);
		std::scoped_lock<decltype(m_mutex)> lock(m_mutex);
		m_loading.push_back(Resources::inst().create<Texture>(std::move(cubemap.id), std::move(cubemap.info)));
	};
	task.name = "Cubemaps";
	task.iterationCount = m_info.cubemaps.size();
	addJobs(task);
}

bool AssetManifest::eraseDone()
{
	std::scoped_lock<decltype(m_mutex)> lock(m_mutex);
	if (!m_running.empty())
	{
		auto iter = std::remove_if(m_running.begin(), m_running.end(), [](auto const& sJob) -> bool { return sJob->hasCompleted(); });
		m_running.erase(iter, m_running.end());
	}
	if (!m_loading.empty())
	{
		auto iter = std::remove_if(m_loading.begin(), m_loading.end(), [](Asset* pAsset) { return pAsset->isReady(); });
		m_loading.erase(iter, m_loading.end());
	}
	return m_running.empty() && m_loading.empty();
}

void AssetManifest::addJobs(IndexedTask const& task)
{
	auto hjobs = jobs::forEach(task);
	std::move(hjobs.begin(), hjobs.end(), std::back_inserter(m_running));
}
} // namespace le
