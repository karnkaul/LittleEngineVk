#pragma once
#include <core/hash.hpp>
#include <core/io/path.hpp>
#include <engine/render/material.hpp>
#include <engine/render/mesh_view.hpp>
#include <graphics/mesh_primitive.hpp>
#include <graphics/texture.hpp>
#include <ktl/expected.hpp>

namespace le {
namespace io {
class Media;
}
namespace graphics {
class Sampler;
}

class Model {
  public:
	enum class Failcode {
		eUnknown,
		eJsonNotFound,
		eJsonMissingData,
		eJsonReadFailure,
		eObjMtlReadFailure,
		eTextureNotFound,
		eObjNotFound,
		eMtlNotFound,
		eTextureCreateFailure,
	};

	struct TexData;
	struct MatData;
	struct MeshData;
	struct CreateInfo;

	using VRAM = graphics::VRAM;
	using Sampler = graphics::Sampler;
	using Texture = graphics::Texture;
	using MeshPrimitive = graphics::MeshPrimitive;
	template <typename T>
	using Result = ktl::expected<T, Failcode>;

	static Result<CreateInfo> load(io::Path modelID, io::Path jsonID, io::Media const& media);

	Result<void> construct(not_null<VRAM*> vram, CreateInfo const& info, Sampler const& sampler, std::optional<vk::Format> forceFormat);

	MeshView mesh() const;

	std::size_t textureCount() const noexcept { return m_storage.textures.size(); }
	std::size_t materialCount() const noexcept { return m_storage.materials.size(); }
	std::size_t primitiveCount() const noexcept { return m_storage.meshMats.size(); }

	MeshPrimitive* primitive(std::size_t index);
	Material* material(std::size_t index);

  private:
	template <typename V>
	using Map = std::unordered_map<Hash, V>;

	struct MeshMat {
		MeshPrimitive primitive;
		Hash mat;
	};

	struct {
		Map<Texture> textures;
		Map<Material> materials;
		std::vector<MeshMat> meshMats;
	} m_storage;
	mutable std::vector<MeshObj> m_meshes;
};

struct Model::TexData {
	io::Path uri;
	io::Path filename;
	bytearray bytes;
	Hash samplerID;
	Hash hash;
};

struct Model::MatData {
	io::Path uri;
	Material mtl;
	std::vector<std::size_t> diffuse;
	std::vector<std::size_t> specular;
	std::vector<std::size_t> alpha;
	std::vector<std::size_t> bump;
	Hash hash;
};

struct Model::MeshData {
	io::Path uri;
	graphics::Geometry geometry;
	std::vector<std::size_t> matIndices;
	Hash hash;
};

struct Model::CreateInfo {
	glm::vec3 origin = {};
	std::vector<MeshData> meshes;
	std::vector<TexData> textures;
	std::vector<MatData> materials;
	io::Path uri;
};
} // namespace le
