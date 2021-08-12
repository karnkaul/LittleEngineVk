#pragma once
#include <core/hash.hpp>
#include <core/io/path.hpp>
#include <engine/scene/prop.hpp>
#include <graphics/mesh.hpp>
#include <graphics/texture.hpp>
#include <ktl/expected.hpp>

namespace le {
namespace io {
class Reader;
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
	using Mesh = graphics::Mesh;
	template <typename T>
	using Result = ktl::expected<T, Error>;

	static Result<CreateInfo> load(io::Path modelID, io::Path jsonID, io::Reader const& reader);

	Result<Span<Prop const>> construct(not_null<VRAM*> vram, CreateInfo const& info, Sampler const& sampler, std::optional<vk::Format> forceFormat);

	Span<Prop const> props() const noexcept { return m_storage.props; }
	Span<Prop> propsRW() noexcept { return m_storage.props; }

  private:
	template <typename V>
	using Map = std::unordered_map<Hash, V>;

	struct {
		Map<graphics::Texture> textures;
		Map<Material> materials;
		Map<graphics::Mesh> meshes;
		std::vector<Prop> props;
	} m_storage;
};

struct Model::TexData {
	io::Path id;
	io::Path filename;
	bytearray bytes;
	Hash samplerID;
	Hash hash;
};

struct Model::MatData {
	io::Path id;
	Material mtl;
	std::vector<std::size_t> diffuse;
	std::vector<std::size_t> specular;
	std::vector<std::size_t> alpha;
	std::vector<std::size_t> bump;
	Hash hash;
};

struct Model::MeshData {
	io::Path id;
	graphics::Geometry geometry;
	std::vector<std::size_t> matIndices;
	Hash hash;
};

struct Model::CreateInfo {
	glm::vec3 origin = {};
	std::vector<MeshData> meshes;
	std::vector<TexData> textures;
	std::vector<MatData> materials;
	io::Path id;
};
} // namespace le
