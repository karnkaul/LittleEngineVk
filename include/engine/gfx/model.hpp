#pragma once
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>
#include <core/hash.hpp>
#include <engine/assets/asset.hpp>
#include <engine/gfx/geometry.hpp>
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
		Hash hash;
	};
	struct MatData final
	{
		stdfs::path id;
		std::vector<std::size_t> diffuseIndices;
		std::vector<std::size_t> specularIndices;
		std::vector<std::size_t> bumpIndices;
		Albedo albedo;
		f32 shininess = 32.0f;
		res::Material::Flags flags;
		Hash hash;
	};
	struct MeshData final
	{
		Geometry geometry;
		stdfs::path id;
		std::vector<std::size_t> materialIndices;
		f32 shininess = 32.0f;
		Hash hash;
	};

	struct Info final
	{
		glm::vec3 origin = glm::vec3(0.0f);
		std::vector<TexData> textures;
		std::vector<MatData> materials;
		std::vector<MeshData> meshData;
		std::vector<res::Mesh> preloaded;
		res::Mesh::Type type = res::Mesh::Type::eStatic;
		res::Texture::Space mode = res::Texture::Space::eSRGBNonLinear;
		Colour tint = colours::white;
		bool bDropColour = false;
	};

public:
	static std::string const s_tName;

public:
	static Info parseOBJ(stdfs::path const& assetID);

public:
	Model(stdfs::path id, Info info);
	~Model();

protected:
	std::deque<res::Material::Inst> m_materials;
	std::vector<res::Mesh> m_meshes;
	std::deque<res::Mesh> m_loadedMeshes;
	std::unordered_map<Hash, res::Material> m_loadedMaterials;
	std::unordered_map<Hash, res::Texture> m_textures;

public:
	std::vector<res::Mesh> meshes() const;
#if defined(LEVK_EDITOR)
	std::deque<res::Mesh>& loadedMeshes();
#endif

public:
	Status update() override;
};
} // namespace le::gfx
