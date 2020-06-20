#include <unordered_map>
#include <unordered_set>
#include <fmt/format.h>
#include <glm/gtx/hash.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
#include <core/assert.hpp>
#include <core/colour.hpp>
#include <core/profiler.hpp>
#include <core/gdata.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <engine/gfx/model.hpp>
#include <gfx/device.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_PROFILE_MODEL_LOADS)
#define LEVK_PROFILE_MODEL_LOADS
#endif
#endif

namespace le::gfx
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
		IOReader const* pReader = nullptr;
		bool bDropColour = false;
	};

public:
	Model::Info m_info;

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
	size_t texIdx(std::string_view texName);
	size_t matIdx(tinyobj::material_t const& fromMat, std::string_view id);
	std::string meshName(tinyobj::shape_t const& shape);
	Geometry vertices(tinyobj::shape_t const& shape);
	std::vector<size_t> materials(tinyobj::shape_t const& shape);
};

glm::vec3 getVec3(GData const& json, std::string const& id, glm::vec3 const& def = glm::vec3(0.0f))
{
	auto vec = json.getVecString(id);
	glm::vec3 ret = def;
	if (vec.size() > 0)
	{
		ret.x = utils::strings::toF32(vec.at(0), def.x);
	}
	if (vec.size() > 1)
	{
		ret.y = utils::strings::toF32(vec.at(1), def.y);
	}
	if (vec.size() > 2)
	{
		ret.z = utils::strings::toF32(vec.at(2), def.z);
	}
	return ret;
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
		m_info.id = m_modelID;
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
			auto [bytes, bResult] = data.pReader->getBytes(texture.filename);
			if (bResult)
			{
				texture.bytes = std::move(bytes);
			}
			else
			{
				LOG_W("[{}] [{}] Failed to load texture [{}] from [{}]", Model::s_tName, idStr, texture.filename.generic_string(),
					  data.pReader->medium());
			}
		}
	}
}

Model::MeshData OBJParser::processShape(tinyobj::shape_t const& shape)
{
	Model::MeshData meshData;
	auto name = meshName(shape);
	meshData.hash = Model::strHash(name);
	meshData.id = std::move(name);
	meshData.geometry = vertices(shape);
	meshData.materialIndices = materials(shape);
	return meshData;
}

size_t OBJParser::texIdx(std::string_view texName)
{
	auto id = fmt::format("{}-{}", m_modelID.generic_string(), texName);
	auto const hash = Model::strHash(id);
	for (size_t idx = 0; idx < m_info.textures.size(); ++idx)
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

size_t OBJParser::matIdx(tinyobj::material_t const& fromMat, std::string_view id)
{
	auto const hash = Model::strHash(id);
	for (size_t idx = 0; idx < m_info.materials.size(); ++idx)
	{
		if (m_info.materials.at(idx).hash == hash)
		{
			return idx;
		}
	}
	Model::MatData mat;
	mat.id = id;
	mat.hash = hash;
	mat.flags.set({Material::Flag::eLit, Material::Flag::eTextured, Material::Flag::eOpaque});
	switch (fromMat.illum)
	{
	default:
		break;
	case 0:
	case 1:
	{
		mat.flags.reset(Material::Flag::eLit);
		break;
	}
	case 4:
	{
		mat.flags.reset(Material::Flag::eOpaque);
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
		mat.flags.reset(Material::Flag::eTextured);
	}
	if (m_bDropColour)
	{
		mat.flags.set(Material::Flag::eDropColour);
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
	std::string ret = fmt::format("{}-{}", m_modelID.generic_string(), shape.name);
	if (m_meshIDs.find(ret) != m_meshIDs.end())
	{
		ret = fmt::format("{}-{}", std::move(ret), m_info.meshData.size());
		LOG_W("[{}] [{}] Duplicate mesh name in [{}]!", Model::s_tName, shape.name, m_modelID.generic_string());
	}
	m_meshIDs.insert(ret);
	return ret;
}

Geometry OBJParser::vertices(tinyobj::shape_t const& shape)
{
	Geometry ret;
	ret.reserve((u32)m_attrib.vertices.size(), (u32)shape.mesh.indices.size());
	std::unordered_map<size_t, u32> hashToVertIdx;
	hashToVertIdx.reserve(shape.mesh.indices.size());
	for (auto const& idx : shape.mesh.indices)
	{
		// clang-format off
		glm::vec3 const p = m_origin * m_scale + glm::vec3{
			m_attrib.vertices.at(3 * (size_t)idx.vertex_index + 0) * m_scale,
			m_attrib.vertices.at(3 * (size_t)idx.vertex_index + 1) * m_scale,
			m_attrib.vertices.at(3 * (size_t)idx.vertex_index + 2) * m_scale
		};
		glm::vec3 const c = {
			m_attrib.colors.at(3 * (size_t)idx.vertex_index + 0),
			m_attrib.colors.at(3 * (size_t)idx.vertex_index + 1),
			m_attrib.colors.at(3 * (size_t)idx.vertex_index + 2)
		};
		glm::vec3 const n = {
			m_attrib.normals.empty() || idx.normal_index < 0 ? 0.0f : m_attrib.normals.at(3 * (size_t)idx.normal_index + 0),
			m_attrib.normals.empty() || idx.normal_index < 0 ? 0.0f : m_attrib.normals.at(3 * (size_t)idx.normal_index + 1),
			m_attrib.normals.empty() || idx.normal_index < 0 ? 0.0f : m_attrib.normals.at(3 * (size_t)idx.normal_index + 2)
		};
		auto const& at = m_attrib.texcoords;
		glm::vec2 const t = {
			at.empty() || idx.texcoord_index < 0 ? 0.0f : at.at(2 * (size_t)idx.texcoord_index + 0),
			1.0f - (at.empty() || idx.texcoord_index < 0 ? 0.0f : at.at(2 * (size_t)idx.texcoord_index + 1)),
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

std::vector<size_t> OBJParser::materials(tinyobj::shape_t const& shape)
{
	std::unordered_set<size_t> uniqueIndices;
	if (!shape.mesh.material_ids.empty())
	{
		for (auto materialIdx : shape.mesh.material_ids)
		{
			if (materialIdx >= 0)
			{
				auto const& fromMat = m_materials.at((size_t)materialIdx);
				std::string const id = fmt::format("{}-{}", m_modelID.generic_string(), fromMat.name);
				uniqueIndices.insert(matIdx(fromMat, id));
			}
		}
	}
	return std::vector<size_t>(uniqueIndices.begin(), uniqueIndices.end());
}
} // namespace

size_t Model::idHash(stdfs::path const& id)
{
	return std::hash<std::string>()(id.generic_string());
}

size_t Model::strHash(std::string_view id)
{
	return std::hash<std::string_view>()(id);
}

Model::Info Model::parseOBJ(LoadRequest const& request)
{
	ASSERT(request.pReader, "Reader is null!");
	if (!request.pReader)
	{
		LOG_E("[{}] Reader is null!", s_tName);
		return {};
	}
	auto jsonID = (request.jsonID / request.jsonID.filename());
	jsonID += ".json";
	auto [jsonStr, bResult] = request.pReader->getString(jsonID);
	if (!bResult)
	{
		LOG_E("[{}] [{}] not found!", s_tName, jsonID.generic_string());
		return {};
	}
	GData json(std::move(jsonStr));
	if (json.fieldCount() == 0 || !json.contains("mtl") || !json.contains("obj"))
	{
		LOG_E("[{}] No data in json: [{}]!", s_tName, request.jsonID.generic_string());
		return {};
	}
	auto const objPath = request.jsonID / json.getString("obj", "");
	auto const mtlPath = request.jsonID / json.getString("mtl", "");
	if (!request.pReader->checkPresence(objPath) || !request.pReader->checkPresence(mtlPath))
	{
		LOG_E("[{}] .OBJ / .MTL data not present in [{}]: [{}], [{}]!", s_tName, request.pReader->medium(), objPath.generic_string(),
			  mtlPath.generic_string());
		return {};
	}
	auto [objBuf, bObjResult] = request.pReader->getStr(objPath);
	auto [mtlBuf, bMtlResult] = request.pReader->getStr(mtlPath);
	if (bObjResult && bMtlResult)
	{
		OBJParser::Data objData;
		objData.objBuf = std::move(objBuf);
		objData.mtlBuf = std::move(mtlBuf);
		objData.jsonID = std::move(jsonID);
		objData.modelID = json.getString("id", "models/UNNAMED");
		objData.samplerID = json.getString("sampler", "samplers/default");
		objData.scale = (f32)json.getF64("scale", 1.0f);
		objData.pReader = request.pReader;
		objData.bDropColour = json.getBool("dropColour", false);
		objData.origin = getVec3(json, "origin");
		OBJParser parser(std::move(objData));
		return std::move(parser.m_info);
	}
	return {};
}

std::string const Model::s_tName = utils::tName<Model>();

Model::Model(stdfs::path id, Info info) : Asset(std::move(id))
{
#if defined(LEVK_PROFILE_MODEL_LOADS)
	Profiler pr(m_id.generic_string());
#endif
	ASSERT(!(info.meshData.empty() && info.preloaded.empty()), "No mesh data!");
	m_meshes = std::move(info.preloaded);
	m_meshes.reserve(m_meshes.size() + info.meshData.size());
	std::copy(info.preloaded.begin(), info.preloaded.end(), std::back_inserter(m_meshes));
	for (auto& texture : info.textures)
	{
		Texture::Info texInfo;
		texInfo.bytes = {std::move(texture.bytes)};
		texInfo.samplerID = std::move(texture.samplerID);
		texInfo.mode = info.mode;
		Texture newTex(texture.id, std::move(texInfo));
		newTex.setup();
		m_loadedTextures.emplace(texture.hash, std::move(newTex));
	}
	for (auto& material : info.materials)
	{
		Material::Info matInfo;
		matInfo.albedo = material.albedo;
		Material newMat(material.id, std::move(matInfo));
		newMat.setup();
		m_loadedMaterials.emplace(material.hash, std::move(newMat));
		Material::Inst newInst;
		newInst.tint = info.tint;
		auto search = m_loadedMaterials.find(material.hash);
		ASSERT(search != m_loadedMaterials.end(), "Invalid material index!");
		newInst.pMaterial = &search->second;
		if (!material.diffuseIndices.empty())
		{
			size_t idx = material.diffuseIndices.front();
			ASSERT(idx < info.textures.size(), "Invalid texture index!");
			auto search = m_loadedTextures.find(info.textures.at(idx).hash);
			ASSERT(search != m_loadedTextures.end(), "Invalid texture index");
			newInst.pDiffuse = &search->second;
		}
		if (!material.specularIndices.empty())
		{
			size_t idx = material.specularIndices.front();
			ASSERT(idx < info.textures.size(), "Invalid texture index!");
			auto search = m_loadedTextures.find(info.textures.at(idx).hash);
			ASSERT(search != m_loadedTextures.end(), "Invalid texture index!");
			newInst.pSpecular = &search->second;
		}
		newInst.flags = material.flags;
		m_materials.push_back(std::move(newInst));
	}
	for (auto& meshData : info.meshData)
	{
		Mesh::Info meshInfo;
		if (!meshData.materialIndices.empty())
		{
			size_t idx = meshData.materialIndices.front();
			ASSERT(idx < m_materials.size(), "Invalid material index!");
			meshInfo.material = m_materials.at(idx);
		}
		meshInfo.geometry = std::move(meshData.geometry);
		meshInfo.type = info.type;
		Mesh newMesh(meshData.id, std::move(meshInfo));
		newMesh.setup();
		m_loadedMeshes.push_back(std::move(newMesh));
		m_meshes.push_back(&m_loadedMeshes.back());
	}
	m_status = Status::eLoading;
}

std::vector<Mesh const*> Model::meshes() const
{
	return m_status == Status::eReady ? m_meshes : std::vector<Mesh const*>();
}

#if defined(LEVK_EDITOR)
std::deque<Mesh>& Model::loadedMeshes()
{
	return m_loadedMeshes;
}
#endif

Asset::Status Model::update()
{
	if (m_meshes.empty())
	{
		m_status = Status::eMoved;
		return m_status;
	}
	std::for_each(m_loadedMeshes.begin(), m_loadedMeshes.end(), [](auto& mesh) { mesh.update(); });
	std::for_each(m_loadedMaterials.begin(), m_loadedMaterials.end(), [](auto& kvp) { kvp.second.update(); });
	std::for_each(m_loadedTextures.begin(), m_loadedTextures.end(), [](auto& kvp) { kvp.second.update(); });
	if (m_status == Status::eLoading)
	{
		bool bMeshes = std::all_of(m_loadedMeshes.begin(), m_loadedMeshes.end(), [](auto const& mesh) { return mesh.isReady(); });
		bool bTextures =
			std::all_of(m_loadedTextures.begin(), m_loadedTextures.end(), [](auto const& kvp) { return kvp.second.isReady(); });
		if (bMeshes && bTextures)
		{
			m_status = Status::eReady;
		}
	}
	return m_status;
}
} // namespace le::gfx
