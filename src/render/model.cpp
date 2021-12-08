#include <fmt/format.h>
#include <tinyobjloader/tiny_obj_loader.h>
#include <core/io/media.hpp>
#include <core/utils/expect.hpp>
#include <dumb_json/json.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/render/model.hpp>
#include <glm/gtx/hash.hpp>
#include <graphics/mesh_primitive.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace le {
namespace {
constexpr glm::vec2 texCoords(Span<f32 const> arr, std::size_t idx, bool invertY) noexcept {
	f32 const y = arr[2 * idx + 1];
	return {arr[2 * idx + 0], invertY ? 1.0f - y : y};
}

constexpr glm::vec3 vec3(Span<f32 const> arr, std::size_t idx) noexcept { return {arr[3 * idx + 0], arr[3 * idx + 1], arr[3 * idx + 2]}; }

constexpr glm::vec2 texCoords(Span<f32 const> arr, int idx, bool invertY) noexcept {
	return arr.empty() || idx < 0 ? glm::vec2(0.0f, 1.0f) : texCoords(arr, (std::size_t)idx, invertY);
}

constexpr glm::vec3 vec3(Span<f32 const> arr, int idx) noexcept { return arr.empty() || idx < 0 ? glm::vec3(0.0f) : vec3(arr, (std::size_t)idx); }

template <typename T>
std::optional<std::size_t> find(T const& arr, Hash hash) noexcept {
	for (std::size_t idx = 0; idx < arr.size(); ++idx) {
		if (arr[idx].hash == hash) { return idx; }
	}
	return std::nullopt;
}

Colour colour(f32 const (&arr)[3], Colour fallback, f32 a = 1.0f) {
	if (arr[0] > 0.0f && arr[1] > 0.0f && arr[2] > 0.0f) { return Colour({arr[0], arr[1], arr[2], a}); }
	return Colour(glm::vec4(fallback.toVec3(), a));
}

glm::vec3 vec3(dj::json const& json, std::string const& id, glm::vec3 const& fallback = glm::vec3(0.0f)) {
	glm::vec3 ret = fallback;
	if (auto const pVec = json.find(id)) {
		if (auto x = pVec->find("x")) { ret.x = x->as<f32>(); }
		if (auto y = pVec->find("x")) { ret.y = y->as<f32>(); }
		if (auto z = pVec->find("z")) { ret.z = z->as<f32>(); }
	}
	return ret;
}
} // namespace

namespace {
class OBJReader final {
  public:
	struct Data final {
		std::stringstream obj;
		std::stringstream mtl;
		io::Path jsonID;
		io::Path modelID;
		io::Path samplerID;
		glm::vec3 origin = glm::vec3(0.0f);
		f32 scale = 1.0f;
		bool invertV = true;
	};

  private:
	using Failcode = Model::Failcode;

	std::stringstream m_obj;
	std::stringstream m_mtl;
	tinyobj::attrib_t m_attrib;
	std::optional<tinyobj::MaterialStreamReader> m_matStrReader;
	io::Path m_modelURI;
	io::Path m_jsonURI;
	io::Path m_samplerURI;
	std::unordered_set<std::string> m_primitiveURIs;
	std::vector<tinyobj::shape_t> m_shapes;
	std::vector<tinyobj::material_t> m_materials;
	glm::vec3 m_origin;
	f32 m_scale = 1.0f;
	bool m_invertV = true;

  public:
	OBJReader(Data data);

	Model::Result<Model::CreateInfo> operator()(io::Media const& media);

  private:
	Model::MeshData processShape(Model::CreateInfo& info, tinyobj::shape_t const& shape);
	std::size_t texIdx(Model::CreateInfo& info, std::string_view texName);
	std::size_t matIdx(Model::CreateInfo& info, tinyobj::material_t const& fromMat, std::string_view id);
	std::string meshName(Model::CreateInfo& info, tinyobj::shape_t const& shape);
	std::vector<std::size_t> materials(Model::CreateInfo& info, tinyobj::shape_t const& shape);
	graphics::Geometry vertices(tinyobj::shape_t const& shape);
};

OBJReader::OBJReader(Data data)
	: m_obj(std::move(data.obj)), m_mtl(std::move(data.mtl)), m_modelURI(std::move(data.modelID)), m_jsonURI(std::move(data.jsonID)),
	  m_samplerURI(std::move(data.samplerID)), m_origin(data.origin), m_scale(data.scale), m_invertV(data.invertV) {}

Model::Result<Model::CreateInfo> OBJReader::operator()(io::Media const& media) {
	auto const idStr = m_jsonURI.generic_string();
	std::string warn, err;
	tinyobj::MaterialStreamReader* msr = nullptr;
	if (m_mtl) {
		m_matStrReader.emplace(m_mtl);
		msr = &*m_matStrReader;
	}
	bool const bOK = tinyobj::LoadObj(&m_attrib, &m_shapes, &m_materials, &warn, &err, &m_obj, msr);
	if (m_shapes.empty()) { return Failcode::eObjMtlReadFailure; }
	if (!warn.empty() && !bOK) { return Failcode::eObjMtlReadFailure; }
	if (!err.empty()) { return Failcode::eObjMtlReadFailure; }
	if (!bOK) { return Failcode::eObjMtlReadFailure; }
	Model::CreateInfo ret;
	for (auto const& shape : m_shapes) { ret.meshes.push_back(processShape(ret, shape)); }
	for (auto& texture : ret.textures) {
		auto bytes = media.bytes(texture.filename);
		if (!bytes.has_value()) { return Failcode::eTextureNotFound; }
		texture.bytes = std::move(bytes).value();
	}
	return Model::Result<Model::CreateInfo>(std::move(ret));
}

Model::MeshData OBJReader::processShape(Model::CreateInfo& info, tinyobj::shape_t const& shape) {
	Model::MeshData meshData;
	meshData.uri = meshName(info, shape);
	meshData.hash = meshData.uri;
	meshData.geometry = vertices(shape);
	meshData.matIndices = materials(info, shape);
	return meshData;
}

std::size_t OBJReader::texIdx(Model::CreateInfo& info, std::string_view texName) {
	auto const id = (m_modelURI / texName).generic_string();
	Hash const hash = id;
	if (auto search = find(info.textures, hash)) { return *search; }
	Model::TexData tex;
	tex.filename = m_jsonURI.parent_path() / texName;
	tex.uri = std::move(id);
	tex.hash = hash;
	tex.samplerID = m_samplerURI;
	info.textures.push_back(std::move(tex));
	return info.textures.size() - 1;
}

std::size_t OBJReader::matIdx(Model::CreateInfo& info, tinyobj::material_t const& fromMat, std::string_view id) {
	Hash const hash = id;
	if (auto search = find(info.materials, hash)) { return *search; }
	Model::MatData mat;
	mat.uri = id;
	mat.hash = hash;
	mat.mtl.illum = fromMat.illum;
	mat.mtl.Ka = colour(fromMat.ambient, colours::white);
	mat.mtl.Kd = colour(fromMat.diffuse, colours::white);
	mat.mtl.Ks = colour(fromMat.specular, colours::black);
	mat.mtl.Tf = colour(fromMat.transmittance, colours::white);
	mat.mtl.Ns = std::min(fromMat.shininess, 0.0f);
	mat.mtl.d = std::clamp(fromMat.dissolve, 0.0f, 1.0f);
	if (!fromMat.diffuse_texname.empty()) { mat.diffuse.push_back(texIdx(info, fromMat.diffuse_texname)); }
	if (!fromMat.specular_texname.empty()) { mat.specular.push_back(texIdx(info, fromMat.specular_texname)); }
	if (!fromMat.bump_texname.empty()) { mat.bump.push_back(texIdx(info, fromMat.bump_texname)); }
	if (!fromMat.alpha_texname.empty()) { mat.alpha.push_back(texIdx(info, fromMat.alpha_texname)); }
	info.materials.push_back(std::move(mat));
	return info.materials.size() - 1;
}

std::string OBJReader::meshName(Model::CreateInfo& info, tinyobj::shape_t const& shape) {
	auto ret = (m_modelURI / shape.name).generic_string();
	if (m_primitiveURIs.find(ret) != m_primitiveURIs.end()) { ret = fmt::format("{}_{}", std::move(ret), info.meshes.size()); }
	m_primitiveURIs.insert(ret);
	return ret;
}

std::vector<std::size_t> OBJReader::materials(Model::CreateInfo& info, tinyobj::shape_t const& shape) {
	std::unordered_set<std::size_t> uniqueIndices;
	if (!shape.mesh.material_ids.empty()) {
		for (int const materialIdx : shape.mesh.material_ids) {
			if (materialIdx >= 0) {
				auto const& fromMat = m_materials[(std::size_t)materialIdx];
				auto const id = (m_modelURI / fromMat.name).generic_string();
				uniqueIndices.insert(matIdx(info, fromMat, id));
			}
		}
	}
	return {uniqueIndices.begin(), uniqueIndices.end()};
}

graphics::Geometry OBJReader::vertices(tinyobj::shape_t const& shape) {
	graphics::Geometry ret;
	ret.reserve((u32)m_attrib.vertices.size(), (u32)shape.mesh.indices.size());
	std::unordered_map<std::size_t, u32> vertIndices;
	vertIndices.reserve(shape.mesh.indices.size());
	auto const fHash = [](Span<f32 const> fs) {
		std::size_t ret{};
		std::size_t offset{};
		for (f32 const f : fs) { ret ^= (std::hash<f32>{}(f) << offset++); }
		return ret;
	};
	auto const vertHash = [fHash](tinyobj::attrib_t const& attrib, tinyobj::index_t const& idx) {
		std::size_t ret = fHash(Span(attrib.vertices.data() + std::size_t(idx.vertex_index), 3));
		ret ^= (fHash(Span(attrib.colors.data() + std::size_t(idx.vertex_index), 3)) << 2);
		if (idx.normal_index >= 0) { ret ^= (fHash(Span(attrib.normals.data() + std::size_t(idx.normal_index), 3)) << 4); }
		if (idx.texcoord_index >= 0) { ret ^= (fHash(Span(attrib.texcoords.data() + std::size_t(idx.texcoord_index), 2)) << 8); }
		return ret;
	};
	for (auto const& idx : shape.mesh.indices) {
		auto const hash = vertHash(m_attrib, idx);
		if (auto const search = vertIndices.find(hash); search != vertIndices.end()) {
			ret.indices.push_back(search->second);
		} else {
			glm::vec3 const p = m_scale * (m_origin + vec3(m_attrib.vertices, (std::size_t)idx.vertex_index));
			glm::vec3 const c = vec3(m_attrib.colors, (std::size_t)idx.vertex_index);
			glm::vec3 const n = vec3(m_attrib.normals, idx.normal_index);
			glm::vec2 const t = texCoords(m_attrib.texcoords, idx.texcoord_index, m_invertV);
			auto const idx = ret.addVertex({p, c, n, t});
			ret.indices.push_back(idx);
			vertIndices.emplace(hash, idx);
		}
	}
	ret.vertices.shrink_to_fit();
	ret.indices.shrink_to_fit();
	return ret;
}
} // namespace

namespace {
graphics::Texture const* texture(std::unordered_map<Hash, graphics::Texture> const& map, Span<Model::TexData const> tex, Span<std::size_t const> indices) {
	if (!indices.empty()) {
		std::size_t const idx = indices.front();
		if (idx < tex.size()) {
			auto const& tx = tex[idx];
			if (auto const it = map.find(tx.hash); it != map.end()) { return &it->second; }
		}
	}
	return nullptr;
}
} // namespace

Model::Result<Model::CreateInfo> Model::load(io::Path modelID, io::Path jsonID, io::Media const& media) {
	auto res = media.string(jsonID);
	if (!res) { return Failcode::eJsonNotFound; }
	dj::json json;
	auto result = json.read(*res);
	if (result.failure || !result.errors.empty() || !json.is_object()) { return Failcode::eJsonMissingData; }
	if (!json.contains("obj")) { return Failcode::eJsonMissingData; }
	auto const jsonDir = jsonID.parent_path();
	auto const objID = jsonDir / json["obj"].as<std::string>();
	auto const mtlID = jsonDir / (json.contains("mtl") ? json["mtl"].as<std::string>() : std::string());
	auto obj = media.sstream(objID);
	auto mtl = media.sstream(mtlID);
	if (!obj) { return Failcode::eObjNotFound; }
	auto pSamplerID = json.find("sampler");
	auto pScale = json.find("scale");
	OBJReader::Data objData;
	objData.obj = std::move(obj).value();
	if (mtl) { objData.mtl = std::move(mtl).value(); }
	objData.modelID = std::move(modelID);
	objData.jsonID = std::move(jsonID);
	objData.modelID = jsonDir;
	objData.samplerID = pSamplerID ? pSamplerID->as<std::string>() : "samplers/default";
	objData.scale = pScale ? pScale->as<f32>() : 1.0f;
	objData.origin = vec3(json, "origin");
	OBJReader parser(std::move(objData));
	return parser(media);
}

Model::Result<void> Model::construct(not_null<VRAM*> vram, CreateInfo const& info, Sampler const& sampler, std::optional<vk::Format> forceFormat) {
	auto storage = decltype(m_storage){};
	for (auto const& tex : info.textures) {
		if (!tex.bytes.empty()) {
			graphics::Texture texture(vram, sampler.sampler());
			if (!texture.construct(tex.bytes, graphics::Texture::Payload::eColour, forceFormat.value_or(graphics::Image::srgb_v))) {
				return Failcode::eTextureCreateFailure;
			}
			storage.textures.emplace(tex.uri, std::move(texture));
		}
	}
	for (auto const& mat : info.materials) {
		Material material = mat.mtl;
		material.map_Kd = texture(storage.textures, info.textures, mat.diffuse);
		material.map_Ks = texture(storage.textures, info.textures, mat.specular);
		material.map_d = texture(storage.textures, info.textures, mat.alpha);
		storage.materials.emplace(mat.hash, material);
	}
	for (auto const& m : info.meshes) {
		MeshMat meshMat{MeshPrimitive(vram), {}};
		meshMat.primitive.construct(m.geometry);
		Hash const hash = info.uri / m.uri;
		if (!m.matIndices.empty()) {
			auto const& mat = info.materials[m.matIndices.front()];
			EXPECT(storage.materials.contains(mat.hash));
			meshMat.mat = mat.hash;
		} else {
			storage.materials.emplace(hash, Material());
			meshMat.mat = hash;
		}
		storage.meshMats.push_back(std::move(meshMat));
	}
	m_storage = std::move(storage);
	return Result<void>::success();
}

MeshView Model::mesh() const {
	m_meshes.clear();
	m_meshes.reserve(m_storage.meshMats.size());
	for (auto const& meshMat : m_storage.meshMats) {
		MeshObj mesh;
		mesh.primitive = &meshMat.primitive;
		EXPECT(meshMat.mat != Hash());
		if (meshMat.mat != Hash()) {
			auto const it = m_storage.materials.find(meshMat.mat);
			EXPECT(it != m_storage.materials.end());
			if (it != m_storage.materials.end()) { mesh.material = &it->second; }
		}
		m_meshes.push_back(mesh);
	}
	return MeshObjView(m_meshes);
}

MeshPrimitive* Model::primitive(std::size_t index) {
	if (index < primitiveCount()) { return &m_storage.meshMats[index].primitive; }
	return {};
}

Material* Model::material(std::size_t index) {
	if (index < primitiveCount()) {
		if (auto it = m_storage.materials.find(m_storage.meshMats[index].mat); it != m_storage.materials.end()) { return &it->second; }
	}
	return {};
}
} // namespace le
