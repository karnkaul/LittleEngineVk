#pragma once
#include <core/hash.hpp>
#include <core/io/path.hpp>
#include <engine/render/prop.hpp>
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
		eTextureCreateFailure
	};

	struct Error {
		std::string message;
		Failcode failcode{};
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
	using Result = ktl::expected<T, Error>;

	static Result<CreateInfo> load(io::Path modelID, io::Path jsonID, io::Media const& media);

	Result<Span<Prop const>> construct(not_null<VRAM*> vram, CreateInfo const& info, Sampler const& sampler, std::optional<vk::Format> forceFormat);

	Span<Prop const> props() const noexcept { return m_storage.props; }
	Span<Prop> propsRW() noexcept { return m_storage.props; }

  private:
	template <typename V>
	using Map = std::unordered_map<Hash, V>;

	struct {
		Map<Texture> textures;
		Map<Material> materials;
		Map<MeshPrimitive> primitives;
		std::vector<Prop> props;
	} m_storage;
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
