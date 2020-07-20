#pragma once
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>
#include <engine/assets/asset.hpp>
#include <engine/gfx/geometry.hpp>
#include <engine/gfx/mesh.hpp>
#include <engine/resources/resource_types.hpp>

namespace le::gfx
{
class Model : public Asset
{
public:
	struct TexData final
	{
		stdfs::path id;
		stdfs::path samplerID;
		stdfs::path filename;
		bytearray bytes;
		std::size_t hash = 0;
	};
	struct MatData final
	{
		stdfs::path id;
		std::vector<std::size_t> diffuseIndices;
		std::vector<std::size_t> specularIndices;
		std::vector<std::size_t> bumpIndices;
		Albedo albedo;
		f32 shininess = 32.0f;
		Material::Flags flags;
		std::size_t hash = 0;
	};
	struct MeshData final
	{
		Geometry geometry;
		stdfs::path id;
		std::vector<std::size_t> materialIndices;
		f32 shininess = 32.0f;
		std::size_t hash = 0;
	};

	struct Info final
	{
		glm::vec3 origin = glm::vec3(0.0f);
		std::vector<TexData> textures;
		std::vector<MatData> materials;
		std::vector<MeshData> meshData;
		std::vector<Mesh const*> preloaded;
		Mesh::Type type = Mesh::Type::eStatic;
		resources::Texture::Space mode = resources::Texture::Space::eSRGBNonLinear;
		Colour tint = colours::white;
		bool bDropColour = false;
	};

	struct LoadRequest final
	{
		stdfs::path assetID;
		io::Reader const* pReader = nullptr;
	};

public:
	static std::string const s_tName;

public:
	static std::size_t idHash(stdfs::path const& id);
	static std::size_t strHash(std::string_view id);
	static Info parseOBJ(LoadRequest const& request);

public:
	Model(stdfs::path id, Info info);
	~Model();

protected:
	std::vector<Mesh const*> m_meshes;
	std::deque<Material::Inst> m_materials;
	std::deque<Mesh> m_loadedMeshes;
	std::unordered_map<std::size_t, Material> m_loadedMaterials;
	std::unordered_map<std::size_t, resources::Texture> m_textures;

public:
	std::vector<Mesh const*> meshes() const;
#if defined(LEVK_EDITOR)
	std::deque<Mesh>& loadedMeshes();
#endif

public:
	Status update() override;
};
} // namespace le::gfx
