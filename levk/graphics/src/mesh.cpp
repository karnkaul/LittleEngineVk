#include <tinyobjloader/tiny_obj_loader.h>
#include <dumb_json/json.hpp>
#include <levk/core/io/media.hpp>
#include <levk/graphics/mesh.hpp>

namespace le::graphics {
namespace {
glm::vec3 vec3(dj::json const& json, std::string const& id, glm::vec3 const& fallback = {}) {
	glm::vec3 ret = fallback;
	if (auto const& vec = json.find(id)) {
		ret.x = vec->get_as<f32>("x");
		ret.y = vec->get_as<f32>("y");
		ret.z = vec->get_as<f32>("z");
	}
	return ret;
}

constexpr glm::vec2 texCoords(Span<f32 const> arr, std::size_t idx, bool invertY) noexcept {
	f32 const y = arr[2 * idx + 1];
	return {arr[2 * idx + 0], invertY ? 1.0f - y : y};
}

constexpr glm::vec3 vec3(Span<f32 const> arr, std::size_t idx) noexcept { return {arr[3 * idx + 0], arr[3 * idx + 1], arr[3 * idx + 2]}; }

constexpr glm::vec2 texCoords(Span<f32 const> arr, int idx, bool invertY) noexcept {
	return arr.empty() || idx < 0 ? glm::vec2(0.0f, 1.0f) : texCoords(arr, (std::size_t)idx, invertY);
}

constexpr glm::vec3 vec3(Span<f32 const> arr, int idx) noexcept { return arr.empty() || idx < 0 ? glm::vec3(0.0f) : vec3(arr, (std::size_t)idx); }

constexpr Colour colour(f32 const (&arr)[3]) noexcept {
	if (arr[0] > 0.0f && arr[1] > 0.0f && arr[2] > 0.0f) { return Colour({arr[0], arr[1], arr[2], 1.0f}); }
	return colours::white;
}

struct ObjMtlData {
	std::string obj{};
	std::string mtl{};
	io::Path dir{};
	glm::vec3 origin{};
	f32 scale{};

	static ObjMtlData make(io::Path const& jsonURI, io::Media const& media) {
		if (jsonURI.empty()) { return {}; }
		auto const jsonStr = media.string(jsonURI);
		if (!jsonStr || jsonStr->empty()) { return {}; }
		dj::json json;
		if (!json.read(*jsonStr)) { return {}; }
		io::Path dir = jsonURI.parent_path();
		ObjMtlData ret;
		auto obj = json["obj"].as<std::string_view>();
		if (obj.empty()) { return {}; }
		auto objStr = media.string(dir / obj);
		if (!objStr) { return {}; }
		ret.obj = std::move(*objStr);
		if (auto mtl = json.find_as<std::string_view>("mtl")) {
			if (auto mtlStr = media.string(dir / *mtl)) { ret.mtl = std::move(*mtlStr); }
		} else {
			auto mtlStr = ret.obj.substr(0, ret.obj.find_last_of('.'));
			mtlStr += ".mtl";
			if (auto str = media.string(dir / mtlStr)) { ret.mtl = std::move(*str); }
		}
		ret.scale = json.get_as<float>("scale", 1.0f);
		ret.origin = vec3(json, "origin");
		ret.dir = std::move(dir);
		return ret;
	}
};

struct ObjMtlLoader {
	template <typename Key, typename Value>
	using UMap = std::unordered_map<Key, Value>;

	not_null<VRAM*> vram;
	io::Media const& media;
	vk::Sampler sampler;
	io::Path dir{};
	glm::vec3 origin{};
	f32 scale = 1.0f;

	void loadTexture(UMap<Hash, Texture>& out, std::string const& uri) const {
		if (uri.empty()) { return; }
		Hash const hash = uri;
		if (out.contains(hash)) { return; }
		auto bytes = media.bytes(dir / uri);
		if (!bytes || bytes->empty()) { return; }
		Texture texture(vram, sampler);
		if (!texture.construct(*bytes)) { return; }
		out.emplace(hash, std::move(texture));
	}

	UMap<Hash, Texture> loadTextures(Span<tinyobj::material_t const> materials) const {
		UMap<Hash, Texture> ret;
		for (auto const& material : materials) {
			loadTexture(ret, material.alpha_texname);
			loadTexture(ret, material.diffuse_texname);
			loadTexture(ret, material.specular_texname);
			loadTexture(ret, material.bump_texname);
		}
		return ret;
	}

	static std::optional<std::size_t> getIndex(UMap<Hash, std::size_t> const& indices, Hash uri) {
		if (auto it = indices.find(uri); it != indices.end()) { return it->second; }
		return std::nullopt;
	}

	UMap<Hash, Mesh::Material> loadMaterials(Span<tinyobj::material_t const> mats, UMap<Hash, std::size_t> const& texs) {
		UMap<Hash, Mesh::Material> ret;
		ret["default"] = {};
		for (auto const& material : mats) {
			Mesh::Material mat;
			BPMaterialData data;
			data.illum = material.illum;
			data.Ka = colour(material.ambient);
			data.Kd = colour(material.diffuse);
			data.Ks = colour(material.specular);
			data.Tf = colour(material.transmittance);
			data.Ns = std::min(material.shininess, 0.0f);
			data.d = std::clamp(material.dissolve, 0.0f, 1.0f);
			mat.data = data;
			mat.textures[MatTexType::eDiffuse] = getIndex(texs, material.diffuse_texname);
			mat.textures[MatTexType::eSpecular] = getIndex(texs, material.specular_texname);
			mat.textures[MatTexType::eAlpha] = getIndex(texs, material.alpha_texname);
			mat.textures[MatTexType::eBump] = getIndex(texs, material.bump_texname);
			ret.emplace(material.name, mat);
		}
		return ret;
	}

	Geometry loadGeometry(tinyobj::attrib_t const& attrib, tinyobj::shape_t const& shape) {
		Geometry ret;
		ret.reserve((u32)attrib.vertices.size(), (u32)shape.mesh.indices.size());
		UMap<std::size_t, u32> vertIndices;
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
			auto const hash = vertHash(attrib, idx);
			if (auto const search = vertIndices.find(hash); search != vertIndices.end()) {
				ret.indices.push_back(search->second);
			} else {
				glm::vec3 const p = scale * (origin + vec3(attrib.vertices, (std::size_t)idx.vertex_index));
				glm::vec4 const c = glm::vec4(vec3(attrib.colors, (std::size_t)idx.vertex_index), 1.0f);
				glm::vec3 const n = vec3(attrib.normals, idx.normal_index);
				glm::vec2 const t = texCoords(attrib.texcoords, idx.texcoord_index, true);
				auto const idx = ret.addVertex({p, c, n, t});
				ret.indices.push_back(idx);
				vertIndices.emplace(hash, idx);
			}
		}
		ret.vertices.shrink_to_fit();
		ret.indices.shrink_to_fit();
		return ret;
	}

	std::vector<MeshPrimitive> loadPrimitives(tinyobj::ObjReader const& reader, UMap<Hash, std::size_t> const& mats) {
		std::vector<MeshPrimitive> ret;
		auto const& materials = reader.GetMaterials();
		for (auto const& shape : reader.GetShapes()) {
			MeshPrimitive primitive(vram);
			primitive.construct(loadGeometry(reader.GetAttrib(), shape));
			if (!shape.mesh.material_ids.empty() && shape.mesh.material_ids[0] >= 0) {
				auto const& mat = materials[std::size_t(shape.mesh.material_ids[0])];
				if (auto it = mats.find(mat.name); it != mats.end()) { primitive.m_material = it->second; }
			} else {
				if (auto it = mats.find("default"); it != mats.end()) { primitive.m_material = it->second; }
			}
			ret.push_back(std::move(primitive));
		}
		return ret;
	}
};
} // namespace

std::optional<Mesh> Mesh::fromObjMtl(io::Path const& jsonURI, io::Media const& media, not_null<VRAM*> vram, vk::Sampler sampler) {
	auto const data = ObjMtlData::make(jsonURI, media);
	if (data.obj.empty()) { return {}; }
	tinyobj::ObjReader reader;
	if (!reader.ParseFromString(data.obj, data.mtl)) { return {}; }
	Mesh ret;
	if (!sampler) {
		ret.samplers.push_back(Sampler(vram->m_device, Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear})));
		sampler = ret.samplers.back().sampler();
	}
	ObjMtlLoader loader{vram, media, sampler, std::move(data.dir), data.origin, data.scale};
	auto textures = loader.loadTextures(reader.GetMaterials());
	std::unordered_map<Hash, std::size_t> indices;
	for (auto& [hash, texture] : textures) {
		indices[hash] = ret.textures.size();
		ret.textures.push_back(std::move(texture));
	}
	auto materials = loader.loadMaterials(reader.GetMaterials(), indices);
	indices.clear();
	for (auto& [hash, mat] : materials) {
		indices[hash] = ret.materials.size();
		ret.materials.push_back(std::move(mat));
	}
	ret.primitives = loader.loadPrimitives(reader, indices);
	return ret;
}

std::vector<DrawPrimitive> Mesh::primitiveViews() const {
	std::vector<DrawPrimitive> ret;
	ret.reserve(primitives.size());
	AddDrawPrimitives<Mesh>{}(*this, std::back_inserter(ret));
	return ret;
}
} // namespace le::graphics
