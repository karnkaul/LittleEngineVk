#include <unordered_map>
#include <unordered_set>
#include <fmt/format.h>
#include <glm/gtx/hash.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
#include <core/assert.hpp>
#include <core/colour.hpp>
#include <core/profiler.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <dumb_json/dumb_json.hpp>
#include <engine/resources/resources.hpp>
#include <gfx/device.hpp>
#include <resources/resources_impl.hpp>
#include <resources/model_impl.hpp>
#include <resources/resources_impl.hpp>
#include <levk_impl.hpp>

namespace le::res
{
namespace
{
class OBJParser final
{
public:
	struct Data final
	{
		std::stringstream objBuf;
		std::stringstream mtlBuf;
		stdfs::path jsonID;
		stdfs::path modelID;
		stdfs::path samplerID;
		glm::vec3 origin = glm::vec3(0.0f);
		f32 scale = 1.0f;
		bool bDropColour = false;
	};

public:
	Model::CreateInfo m_info;

private:
	tinyobj::attrib_t m_attrib;
	tinyobj::MaterialStreamReader m_matStrReader;
	stdfs::path m_modelID;
	stdfs::path m_jsonID;
	stdfs::path m_samplerID;
	std::unordered_set<std::string> m_meshIDs;
	std::vector<tinyobj::shape_t> m_shapes;
	std::vector<tinyobj::material_t> m_materials;
	glm::vec3 m_origin;
	f32 m_scale = 1.0f;
	bool m_bDropColour = false;

public:
	OBJParser(Data data);

private:
	Model::MeshData processShape(tinyobj::shape_t const& shape);
	std::size_t texIdx(std::string_view texName);
	std::size_t matIdx(tinyobj::material_t const& fromMat, std::string_view id);
	std::string meshName(tinyobj::shape_t const& shape);
	gfx::Geometry vertices(tinyobj::shape_t const& shape);
	std::vector<std::size_t> materials(tinyobj::shape_t const& shape);
};

glm::vec3 getVec3(dj::object const& json, std::string const& id, glm::vec3 const& def = glm::vec3(0.0f))
{
	if (auto const pVec = json.find<dj::object>(id))
	{
		return {(f32)pVec->value<dj::floating>("x"), (f32)pVec->value<dj::floating>("y"), (f32)pVec->value<dj::floating>("z")};
	}
	return def;
}

OBJParser::OBJParser(Data data)
	: m_matStrReader(data.mtlBuf),
	  m_modelID(std::move(data.modelID)),
	  m_jsonID(std::move(data.jsonID)),
	  m_samplerID(std::move(data.samplerID)),
	  m_origin(data.origin),
	  m_scale(data.scale),
	  m_bDropColour(data.bDropColour)
{
	auto const idStr = m_jsonID.generic_string();
	std::string warn, err;
	bool bOK = false;
	{
#if defined(LEVK_PROFILE_MODEL_LOADS)
		Profiler pr(idStr + "-TinyObj");
#endif
		bOK = tinyobj::LoadObj(&m_attrib, &m_shapes, &m_materials, &warn, &err, &data.objBuf, &m_matStrReader);
	}
	if (m_shapes.empty())
	{
		bOK = false;
#if defined(LEVK_DEBUG)
		std::string msg = "No shapes parsed! Perhaps passed OBJ data is empty?";
#else
		std::string msg = "No shapes parsed!";
#endif
		LOG_W("[{}] [{}] {}", Model::s_tName, idStr, msg);
	}
	if (!warn.empty())
	{
		LOG_W("[{}] {}", Model::s_tName, warn);
	}
	if (!err.empty())
	{
		LOG_E("[{}] {}", Model::s_tName, err);
	}
	if (bOK)
	{
		{
#if defined(LEVK_PROFILE_MODEL_LOADS)
			Profiler pr(idStr + "-MeshData");
#endif
			for (auto const& shape : m_shapes)
			{
				m_info.meshData.push_back(processShape(shape));
			}
		}
#if defined(LEVK_PROFILE_MODEL_LOADS)
		Profiler pr(idStr + "-TexData");
#endif
		for (auto& texture : m_info.textures)
		{
			if (auto bytes = engine::reader().bytes(texture.filename))
			{
				texture.bytes = std::move(*bytes);
			}
			else
			{
				LOG_W("[{}] [{}] Failed to load texture [{}] from [{}]", Model::s_tName, idStr, texture.filename.generic_string(), engine::reader().medium());
			}
		}
	}
}

Model::MeshData OBJParser::processShape(tinyobj::shape_t const& shape)
{
	Model::MeshData meshData;
	auto name = meshName(shape);
	meshData.hash = name;
	meshData.id = std::move(name);
	meshData.geometry = vertices(shape);
	meshData.materialIndices = materials(shape);
	return meshData;
}

std::size_t OBJParser::texIdx(std::string_view texName)
{
	auto const id = (m_modelID / texName).generic_string();
	Hash const hash = id;
	for (std::size_t idx = 0; idx < m_info.textures.size(); ++idx)
	{
		if (m_info.textures.at(idx).hash == hash)
		{
			return idx;
		}
	}
	Model::TexData tex;
	tex.filename = m_jsonID.parent_path() / texName;
	tex.id = std::move(id);
	tex.hash = hash;
	tex.samplerID = m_samplerID;
	m_info.textures.push_back(std::move(tex));
	return m_info.textures.size() - 1;
}

std::size_t OBJParser::matIdx(tinyobj::material_t const& fromMat, std::string_view id)
{
	Hash const hash = id;
	for (std::size_t idx = 0; idx < m_info.materials.size(); ++idx)
	{
		if (m_info.materials.at(idx).hash == hash)
		{
			return idx;
		}
	}
	Model::MatData mat;
	mat.id = id;
	mat.hash = hash;
	mat.flags.set({res::Material::Flag::eLit, res::Material::Flag::eTextured, res::Material::Flag::eOpaque});
	switch (fromMat.illum)
	{
	default:
		break;
	case 0:
	case 1:
	{
		mat.flags.reset(res::Material::Flag::eLit);
		break;
	}
	case 4:
	{
		mat.flags.reset(res::Material::Flag::eOpaque);
		break;
	}
	}
	mat.albedo.ambient = Colour({fromMat.ambient[0], fromMat.ambient[1], fromMat.ambient[2]});
	mat.albedo.diffuse = Colour({fromMat.diffuse[0], fromMat.diffuse[1], fromMat.diffuse[2]});
	mat.albedo.specular = Colour({fromMat.specular[0], fromMat.specular[1], fromMat.specular[2]});
	if (fromMat.shininess >= 0.0f)
	{
		mat.shininess = fromMat.shininess;
	}
	if (fromMat.diffuse_texname.empty())
	{
		mat.flags.reset(res::Material::Flag::eTextured);
	}
	if (m_bDropColour)
	{
		mat.flags.set(res::Material::Flag::eDropColour);
	}
	if (!fromMat.diffuse_texname.empty())
	{
		mat.diffuseIndices.push_back(texIdx(fromMat.diffuse_texname));
	}
	if (!fromMat.specular_texname.empty())
	{
		mat.specularIndices.push_back(texIdx(fromMat.specular_texname));
	}
	if (!fromMat.bump_texname.empty())
	{
		mat.bumpIndices.push_back(texIdx(fromMat.bump_texname));
	}
	m_info.materials.push_back(std::move(mat));
	return m_info.materials.size() - 1;
}

std::string OBJParser::meshName(tinyobj::shape_t const& shape)
{
	auto ret = (m_modelID / shape.name).generic_string();
	if (m_meshIDs.find(ret) != m_meshIDs.end())
	{
		ret = fmt::format("{}_{}", std::move(ret), m_info.meshData.size());
		LOG_W("[{}] [{}] Duplicate mesh name in [{}]!", Model::s_tName, shape.name, m_modelID.generic_string());
	}
	m_meshIDs.insert(ret);
	return ret;
}

gfx::Geometry OBJParser::vertices(tinyobj::shape_t const& shape)
{
	gfx::Geometry ret;
	ret.reserve((u32)m_attrib.vertices.size(), (u32)shape.mesh.indices.size());
	std::unordered_map<std::size_t, u32> hashToVertIdx;
	hashToVertIdx.reserve(shape.mesh.indices.size());
	for (auto const& idx : shape.mesh.indices)
	{
		// clang-format off
		glm::vec3 const p = m_origin * m_scale + glm::vec3{
			m_attrib.vertices.at(3 * (std::size_t)idx.vertex_index + 0) * m_scale,
			m_attrib.vertices.at(3 * (std::size_t)idx.vertex_index + 1) * m_scale,
			m_attrib.vertices.at(3 * (std::size_t)idx.vertex_index + 2) * m_scale
		};
		glm::vec3 const c = {
			m_attrib.colors.at(3 * (std::size_t)idx.vertex_index + 0),
			m_attrib.colors.at(3 * (std::size_t)idx.vertex_index + 1),
			m_attrib.colors.at(3 * (std::size_t)idx.vertex_index + 2)
		};
		glm::vec3 const n = {
			m_attrib.normals.empty() || idx.normal_index < 0 ? 0.0f : m_attrib.normals.at(3 * (std::size_t)idx.normal_index + 0),
			m_attrib.normals.empty() || idx.normal_index < 0 ? 0.0f : m_attrib.normals.at(3 * (std::size_t)idx.normal_index + 1),
			m_attrib.normals.empty() || idx.normal_index < 0 ? 0.0f : m_attrib.normals.at(3 * (std::size_t)idx.normal_index + 2)
		};
		auto const& at = m_attrib.texcoords;
		glm::vec2 const t = {
			at.empty() || idx.texcoord_index < 0 ? 0.0f : at.at(2 * (std::size_t)idx.texcoord_index + 0),
			1.0f - (at.empty() || idx.texcoord_index < 0 ? 0.0f : at.at(2 * (std::size_t)idx.texcoord_index + 1)),
		};
		// clang-format on
		auto const hash = std::hash<glm::vec3>()(p) ^ std::hash<glm::vec3>()(n) ^ std::hash<glm::vec3>()(c) ^ std::hash<glm::vec2>()(t);
		if (auto search = hashToVertIdx.find(hash); search != hashToVertIdx.end())
		{
			ret.indices.push_back((u32)search->second);
		}
		else
		{
			auto const idx = ret.addVertex({p, c, n, t});
			ret.indices.push_back(idx);
			hashToVertIdx[hash] = idx;
		}
	}
	ret.vertices.shrink_to_fit();
	ret.indices.shrink_to_fit();
	return ret;
}

std::vector<std::size_t> OBJParser::materials(tinyobj::shape_t const& shape)
{
	std::unordered_set<std::size_t> uniqueIndices;
	if (!shape.mesh.material_ids.empty())
	{
		for (auto materialIdx : shape.mesh.material_ids)
		{
			if (materialIdx >= 0)
			{
				auto const& fromMat = m_materials.at((std::size_t)materialIdx);
				auto const id = (m_modelID / fromMat.name).generic_string();
				uniqueIndices.insert(matIdx(fromMat, id));
			}
		}
	}
	return std::vector<std::size_t>(uniqueIndices.begin(), uniqueIndices.end());
}
} // namespace

Model::Info const& Model::info() const
{
	return res::info(*this);
}

Status Model::status() const
{
	return res::status(*this);
}

template <>
TResult<Model::CreateInfo> LoadBase<Model>::createInfo() const
{
	auto pThis = static_cast<Model::LoadInfo const*>(this);
	auto const jsonDir = pThis->jsonDirectory.empty() ? pThis->idRoot : pThis->jsonDirectory;
	if (jsonDir.empty())
	{
		LOG_E("[{}] Empty resource ID!", Model::s_tName);
		return {};
	}
	auto jsonFile = pThis->jsonFilename;
	if (jsonFile.empty())
	{
		jsonFile = jsonDir.filename().generic_string();
		jsonFile += ".json";
	}
	auto const jsonID = jsonDir / jsonFile;
	auto jsonStr = engine::reader().string(jsonID);
	if (!jsonStr)
	{
		LOG_E("[{}] [{}] not found!", Model::s_tName, jsonID.generic_string());
		return {};
	}
	dj::object json;
	if (!json.read(*jsonStr) || json.fields.empty())
	{
		LOG_E("[{}] Failed to read json: [{}]!", Model::s_tName, jsonID.generic_string());
	}
	auto const& obj = json.value<dj::string>("obj");
	auto const& mtl = json.value<dj::string>("mtl");
	if (mtl.empty() || obj.empty())
	{
		LOG_E("[{}] No data in json: [{}]!", Model::s_tName, jsonID.generic_string());
		return {};
	}
	auto const objPath = jsonDir / obj;
	auto const mtlPath = jsonDir / mtl;
	if (!engine::reader().checkPresence(objPath) || !engine::reader().checkPresence(mtlPath))
	{
		LOG_E("[{}] .OBJ / .MTL data not present in [{}]: [{}], [{}]!", Model::s_tName, engine::reader().medium(), objPath.generic_string(),
			  mtlPath.generic_string());
		return {};
	}
	auto objBuf = engine::reader().sstream(objPath);
	auto mtlBuf = engine::reader().sstream(mtlPath);
	if (objBuf && mtlBuf)
	{
		auto pSamplerID = json.find<dj::string>("sampler");
		auto pScale = json.find<dj::floating>("scale");
		OBJParser::Data objData;
		objData.objBuf = std::move(*objBuf);
		objData.mtlBuf = std::move(*mtlBuf);
		objData.jsonID = std::move(jsonID);
		objData.modelID = pThis->idRoot.empty() ? jsonDir : pThis->idRoot;
		objData.samplerID = pSamplerID ? pSamplerID->value : "samplers/default";
		objData.scale = pScale ? (f32)pScale->value : 1.0f;
		objData.bDropColour = json.value<dj::boolean>("dropColour");
		objData.origin = getVec3(json, "origin");
		OBJParser parser(std::move(objData));
		return std::move(parser.m_info);
	}
	return {};
}

std::vector<Mesh> Model::meshes() const
{
	if (auto pImpl = impl(*this))
	{
		return pImpl->meshes();
	}
	return {};
}

bool Model::Impl::make(CreateInfo& out_createInfo, Info& out_info)
{
#if defined(LEVK_PROFILE_MODEL_LOADS)
	Profiler pr(m_id.generic_string());
#endif
	ASSERT(!(out_createInfo.meshData.empty() && out_createInfo.preloaded.empty()), "No mesh data!");
	m_meshes = std::move(out_createInfo.preloaded);
	m_meshes.reserve(m_meshes.size() + out_createInfo.meshData.size());
	for (auto& texture : out_createInfo.textures)
	{
		res::Texture::CreateInfo texInfo;
		texInfo.bytes = {std::move(texture.bytes)};
		texInfo.samplerID = std::move(texture.samplerID);
		texInfo.mode = out_createInfo.mode;
		auto newTex = res::load(texture.id, std::move(texInfo));
		m_textures.emplace(texture.hash, std::move(newTex));
	}
	for (auto& material : out_createInfo.materials)
	{
		res::Material::CreateInfo matInfo;
		matInfo.albedo = material.albedo;
		auto newMat = res::load(material.id, std::move(matInfo));
		m_loadedMaterials.emplace(material.hash, std::move(newMat));
		res::Material::Inst newInst;
		newInst.tint = out_createInfo.tint;
		auto search = m_loadedMaterials.find(material.hash);
		ASSERT(search != m_loadedMaterials.end(), "Invalid material index!");
		newInst.material = search->second;
		if (!material.diffuseIndices.empty())
		{
			std::size_t idx = material.diffuseIndices.front();
			ASSERT(idx < out_createInfo.textures.size(), "Invalid texture index!");
			auto search = m_textures.find(out_createInfo.textures.at(idx).hash);
			ASSERT(search != m_textures.end(), "Invalid texture index");
			newInst.diffuse = search->second;
		}
		if (!material.specularIndices.empty())
		{
			std::size_t idx = material.specularIndices.front();
			ASSERT(idx < out_createInfo.textures.size(), "Invalid texture index!");
			auto search = m_textures.find(out_createInfo.textures.at(idx).hash);
			ASSERT(search != m_textures.end(), "Invalid texture index!");
			newInst.specular = search->second;
		}
		newInst.flags = material.flags;
		m_materials.push_back(std::move(newInst));
	}
	for (auto& meshData : out_createInfo.meshData)
	{
		res::Mesh::CreateInfo meshInfo;
		if (!meshData.materialIndices.empty())
		{
			std::size_t idx = meshData.materialIndices.front();
			ASSERT(idx < m_materials.size(), "Invalid material index!");
			meshInfo.material = m_materials.at(idx);
		}
		meshInfo.geometry = std::move(meshData.geometry);
		meshInfo.type = out_createInfo.type;
		auto newMesh = res::load(meshData.id, std::move(meshInfo));
		m_loadedMeshes.push_back(std::move(newMesh));
		m_meshes.push_back(m_loadedMeshes.back());
	}
	out_info.origin = out_createInfo.origin;
	out_info.type = out_createInfo.type;
	out_info.mode = out_createInfo.mode;
	out_info.tint = out_createInfo.tint;
	return true;
}

void Model::Impl::release()
{
	for (auto const& [_, texture] : m_textures)
	{
		res::unload(texture);
	}
	for (auto const& [_, material] : m_loadedMaterials)
	{
		res::unload(material);
	}
	for (auto const& mesh : m_loadedMeshes)
	{
		res::unload(mesh);
	}
}

bool Model::Impl::update()
{
	switch (status)
	{
	case Status::eLoading:
	case Status::eReloading:
	{
		bool const bMeshes = std::all_of(m_loadedMeshes.begin(), m_loadedMeshes.end(), [](auto const& mesh) { return mesh.status() == res::Status::eReady; });
		bool const bTextures = std::all_of(m_textures.begin(), m_textures.end(), [](auto const& kvp) { return kvp.second.status() == res::Status::eReady; });
		bool const bMaterials =
			std::all_of(m_loadedMaterials.begin(), m_loadedMaterials.end(), [](auto const& kvp) { return kvp.second.status() == res::Status::eReady; });
		return bMeshes && bTextures && bMaterials;
	}
	default:
	{
		return true;
	}
	}
}

std::vector<res::Mesh> Model::Impl::meshes() const
{
	Model model;
	model.guid = guid;
	return model.status() == Status::eReady ? m_meshes : std::vector<res::Mesh>();
}

#if defined(LEVK_EDITOR)
std::deque<res::Mesh>& Model::Impl::loadedMeshes()
{
	return m_loadedMeshes;
}
#endif
} // namespace le::res
