#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <fmt/format.h>
#include <tinyobjloader/tiny_obj_loader.h>
#include <core/io/reader.hpp>
#include <dumb_json/djson.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/render/model.hpp>
#include <graphics/mesh.hpp>
#include <graphics/texture.hpp>

#if !defined(__ANDROID__)
#include <glm/gtx/hash.hpp>
#endif

#if defined(__ANDROID__)
namespace std {
namespace {
struct fhasher {
	std::size_t i = 0;

	std::size_t operator()(float f) noexcept {
		return std::hash<float>{}(f) << i++;
	}
};
} // namespace

template <>
struct hash<glm::vec2> {
	size_t operator()(glm::vec2 const& v) const noexcept {
		fhasher f;
		return f(v.x) ^ f(v.y);
	}
};
template <>
struct hash<glm::vec3> {
	size_t operator()(glm::vec3 const& v) const noexcept {
		fhasher f;
		return f(v.x) ^ f(v.y) ^ f(v.z);
	}
};
} // namespace std
#endif

namespace le {
namespace {
constexpr glm::vec2 texCoords(View<f32> arr, std::size_t idx, bool invertY) noexcept {
	f32 const y = arr[2 * idx + 1];
	return {arr[2 * idx + 0], invertY ? 1.0f - y : y};
}

constexpr glm::vec3 vec3(View<f32> arr, std::size_t idx) noexcept {
	return {arr[3 * idx + 0], arr[3 * idx + 1], arr[3 * idx + 2]};
}

constexpr glm::vec2 texCoords(View<f32> arr, int idx, bool invertY) noexcept {
	return arr.empty() || idx < 0 ? glm::vec2(0.0f, 1.0f) : texCoords(arr, (std::size_t)idx, invertY);
}

constexpr glm::vec3 vec3(View<f32> arr, int idx) noexcept {
	return arr.empty() || idx < 0 ? glm::vec3(0.0f) : vec3(arr, (std::size_t)idx);
}

template <typename T>
kt::result<std::size_t> find(T const& arr, Hash hash) noexcept {
	for (std::size_t idx = 0; idx < arr.size(); ++idx) {
		if (arr[idx].hash == hash) {
			return idx;
		}
	}
	return kt::null_result;
}

Colour colour(f32 const (&arr)[3], Colour fallback, f32 a = 1.0f) {
	if (arr[0] > 0.0f && arr[1] > 0.0f && arr[2] > 0.0f) {
		return Colour({arr[0], arr[1], arr[2], a});
	}
	return Colour(glm::vec4(fallback.toVec3(), a));
}

glm::vec3 vec3(dj::node_t const& json, std::string const& id, glm::vec3 const& fallback = glm::vec3(0.0f)) {
	if (auto const pVec = json.find(id)) {
		return {pVec->safe_get("x").as<f32>(), pVec->safe_get("y").as<f32>(), pVec->safe_get("z").as<f32>()};
	}
	return fallback;
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
	struct IncrHasher {
		std::size_t count = 0;

		template <typename T>
		std::size_t operator()(T const& t) noexcept {
			return std::hash<T>{}(t) << count++;
		}
	};

	std::stringstream m_obj;
	std::stringstream m_mtl;
	tinyobj::attrib_t m_attrib;
	std::optional<tinyobj::MaterialStreamReader> m_matStrReader;
	io::Path m_modelID;
	io::Path m_jsonID;
	io::Path m_samplerID;
	std::unordered_set<std::string> m_meshIDs;
	std::vector<tinyobj::shape_t> m_shapes;
	std::vector<tinyobj::material_t> m_materials;
	glm::vec3 m_origin;
	f32 m_scale = 1.0f;
	bool m_invertV = true;

  public:
	OBJReader(Data data);

	Model::Result<Model::CreateInfo> operator()(io::Reader const& reader);

  private:
	Model::MeshData processShape(Model::CreateInfo& info, tinyobj::shape_t const& shape);
	std::size_t texIdx(Model::CreateInfo& info, std::string_view texName);
	std::size_t matIdx(Model::CreateInfo& info, tinyobj::material_t const& fromMat, std::string_view id);
	std::string meshName(Model::CreateInfo& info, tinyobj::shape_t const& shape);
	std::vector<std::size_t> materials(Model::CreateInfo& info, tinyobj::shape_t const& shape);
	graphics::Geometry vertices(tinyobj::shape_t const& shape);
};

OBJReader::OBJReader(Data data)
	: m_obj(std::move(data.obj)), m_mtl(std::move(data.mtl)), m_modelID(std::move(data.modelID)), m_jsonID(std::move(data.jsonID)),
	  m_samplerID(std::move(data.samplerID)), m_origin(data.origin), m_scale(data.scale), m_invertV(data.invertV) {
}

Model::Result<Model::CreateInfo> OBJReader::operator()(io::Reader const& reader) {
	auto const idStr = m_jsonID.generic_string();
	std::string warn, err;
	tinyobj::MaterialStreamReader* msr = nullptr;
	if (m_mtl) {
		m_matStrReader.emplace(m_mtl);
		msr = &*m_matStrReader;
	}
	bool const bOK = tinyobj::LoadObj(&m_attrib, &m_shapes, &m_materials, &warn, &err, &m_obj, msr);
	if (m_shapes.empty()) {
		return std::string("No shapes parsed!");
	}
	if (!warn.empty() && !bOK) {
		return warn;
	}
	if (!err.empty()) {
		return err;
	}
	if (!bOK) {
		return std::string("Unknown error(s)");
	}
	Model::CreateInfo ret;
	for (auto const& shape : m_shapes) {
		ret.meshes.push_back(processShape(ret, shape));
	}
	for (auto& texture : ret.textures) {
		auto bytes = reader.bytes(texture.filename);
		ENSURE(bytes.has_result(), "Texture not found!");
		if (bytes) {
			texture.bytes = bytes.move();
		}
	}
	return Model::Result<Model::CreateInfo>(std::move(ret));
}

Model::MeshData OBJReader::processShape(Model::CreateInfo& info, tinyobj::shape_t const& shape) {
	Model::MeshData meshData;
	meshData.id = meshName(info, shape);
	meshData.hash = meshData.id;
	meshData.geometry = vertices(shape);
	meshData.matIndices = materials(info, shape);
	return meshData;
}

std::size_t OBJReader::texIdx(Model::CreateInfo& info, std::string_view texName) {
	auto const id = (m_modelID / texName).generic_string();
	Hash const hash = id;
	if (auto search = find(info.textures, hash)) {
		return *search;
	}
	Model::TexData tex;
	tex.filename = m_jsonID.parent_path() / texName;
	tex.id = std::move(id);
	tex.hash = hash;
	tex.samplerID = m_samplerID;
	info.textures.push_back(std::move(tex));
	return info.textures.size() - 1;
}

std::size_t OBJReader::matIdx(Model::CreateInfo& info, tinyobj::material_t const& fromMat, std::string_view id) {
	Hash const hash = id;
	if (auto search = find(info.materials, hash)) {
		return *search;
	}
	Model::MatData mat;
	mat.id = id;
	mat.hash = hash;
	mat.mtl.illum = fromMat.illum;
	mat.mtl.Ka = colour(fromMat.ambient, colours::white);
	mat.mtl.Kd = colour(fromMat.diffuse, colours::white);
	mat.mtl.Ks = colour(fromMat.specular, colours::black);
	mat.mtl.Tf = colour(fromMat.transmittance, colours::white);
	mat.mtl.Ns = std::min(fromMat.shininess, 0.0f);
	mat.mtl.d = std::clamp(fromMat.dissolve, 0.0f, 1.0f);
	if (!fromMat.diffuse_texname.empty()) {
		mat.diffuse.push_back(texIdx(info, fromMat.diffuse_texname));
	}
	if (!fromMat.specular_texname.empty()) {
		mat.specular.push_back(texIdx(info, fromMat.specular_texname));
	}
	if (!fromMat.bump_texname.empty()) {
		mat.bump.push_back(texIdx(info, fromMat.bump_texname));
	}
	if (!fromMat.alpha_texname.empty()) {
		mat.alpha.push_back(texIdx(info, fromMat.alpha_texname));
	}
	info.materials.push_back(std::move(mat));
	return info.materials.size() - 1;
}

std::string OBJReader::meshName(Model::CreateInfo& info, tinyobj::shape_t const& shape) {
	auto ret = (m_modelID / shape.name).generic_string();
	if (m_meshIDs.find(ret) != m_meshIDs.end()) {
		ret = fmt::format("{}_{}", std::move(ret), info.meshes.size());
	}
	m_meshIDs.insert(ret);
	return ret;
}

std::vector<std::size_t> OBJReader::materials(Model::CreateInfo& info, tinyobj::shape_t const& shape) {
	std::unordered_set<std::size_t> uniqueIndices;
	if (!shape.mesh.material_ids.empty()) {
		for (int const materialIdx : shape.mesh.material_ids) {
			if (materialIdx >= 0) {
				auto const& fromMat = m_materials[(std::size_t)materialIdx];
				auto const id = (m_modelID / fromMat.name).generic_string();
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
	for (auto const& idx : shape.mesh.indices) {
		glm::vec3 const p = m_scale * (m_origin + vec3(m_attrib.vertices, (std::size_t)idx.vertex_index));
		glm::vec3 const c = vec3(m_attrib.colors, (std::size_t)idx.vertex_index);
		glm::vec3 const n = vec3(m_attrib.normals, idx.normal_index);
		glm::vec2 const t = texCoords(m_attrib.texcoords, idx.texcoord_index, m_invertV);
		IncrHasher inc;
		std::size_t const hash = inc(p) ^ inc(c) ^ inc(n) ^ inc(t);
		if (auto const search = vertIndices.find(hash); search != vertIndices.end()) {
			ret.indices.push_back(search->second);
		} else {
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
graphics::Texture const* texture(std::unordered_map<Hash, graphics::Texture> const& map, View<Model::TexData> tex, View<std::size_t> indices) {
	if (!indices.empty()) {
		std::size_t const idx = indices.front();
		if (idx < tex.size()) {
			auto const& tx = tex[idx];
			if (auto const it = map.find(tx.hash); it != map.end()) {
				return &it->second;
			}
		}
	}
	return nullptr;
}
} // namespace

Model::Result<Model::CreateInfo> Model::load(io::Path modelID, io::Path jsonID, io::Reader const& reader) {
	auto res = reader.string(jsonID);
	if (!res) {
		return std::string("JSON not found");
	}
	auto json = dj::node_t::make(*res);
	if (!json || !json->is_object()) {
		return std::string("Failed to read json");
	}
	if (!json->contains("obj")) {
		return std::string("JSON missing obj");
	}
	auto const jsonDir = jsonID.parent_path();
	auto const objID = jsonDir / json->get("obj").as<std::string>();
	auto const mtlID = jsonDir / json->safe_get("mtl").as<std::string>();
	auto obj = reader.sstream(objID);
	auto mtl = reader.sstream(mtlID);
	if (!obj) {
		return std::string("obj not found");
	}
	auto pSamplerID = json->find("sampler");
	auto pScale = json->find("scale");
	OBJReader::Data objData;
	objData.obj = obj.move();
	if (mtl) {
		objData.mtl = mtl.move();
	}
	objData.modelID = std::move(modelID);
	objData.jsonID = std::move(jsonID);
	objData.modelID = jsonDir;
	objData.samplerID = pSamplerID ? pSamplerID->as<std::string>() : "samplers/default";
	objData.scale = pScale ? pScale->as<f32>() : 1.0f;
	objData.origin = vec3(*json, "origin");
	OBJReader parser(std::move(objData));
	return parser(reader);
}

Model::Result<View<Primitive>> Model::construct(not_null<VRAM*> vram, CreateInfo const& info, Sampler const& sampler, vk::Format texFormat) {
	Map<Material> materials;
	decltype(m_storage) storage;
	for (auto const& tex : info.textures) {
		if (!tex.bytes.empty()) {
			graphics::Texture::CreateInfo tci;
			tci.format = texFormat;
			tci.sampler = sampler.sampler();
			tci.data = graphics::Texture::Img{{tex.bytes.begin(), tex.bytes.end()}};
			graphics::Texture texture(vram);
			if (!texture.construct(tci)) {
				return std::string("Failed to construct texture");
			}
			storage.textures.emplace(tex.id, std::move(texture));
		}
	}
	for (auto const& mat : info.materials) {
		Material material = mat.mtl;
		material.map_Kd = texture(storage.textures, info.textures, mat.diffuse);
		material.map_Ks = texture(storage.textures, info.textures, mat.specular);
		material.map_d = texture(storage.textures, info.textures, mat.alpha);
		materials.emplace(mat.hash, material);
	}
	for (auto const& m : info.meshes) {
		graphics::Mesh mesh(vram);
		mesh.construct(m.geometry);
		auto [it, _] = storage.meshes.emplace((info.id / m.id).generic_string(), std::move(mesh));
		Primitive prim;
		prim.mesh = &it->second;
		if (!m.matIndices.empty()) {
			auto const& mat = info.materials[m.matIndices.front()];
			auto const it = materials.find(mat.hash);
			ENSURE(it != materials.end(), "Invalid hash");
			if (it != materials.end()) {
				prim.material = it->second;
			}
		}
		storage.primitives.push_back(prim);
	}
	m_storage = std::move(storage);
	return View<Primitive>(m_storage.primitives);
}
} // namespace le
