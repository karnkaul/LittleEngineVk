#pragma once
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>
#include <engine/assets/asset.hpp>
#include <engine/gfx/geometry.hpp>
#include <engine/gfx/mesh.hpp>
#include <engine/gfx/texture.hpp>

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
		size_t hash = 0;
	};
	struct MatData final
	{
		stdfs::path id;
		std::vector<size_t> diffuseIndices;
		std::vector<size_t> specularIndices;
		std::vector<size_t> bumpIndices;
		Albedo albedo;
		f32 shininess = 32.0f;
		Material::Flags flags;
		size_t hash = 0;
	};
	struct MeshData final
	{
		Geometry geometry;
		stdfs::path id;
		std::vector<size_t> materialIndices;
		f32 shininess = 32.0f;
		size_t hash = 0;
	};

	struct Info final
	{
		stdfs::path id;
		std::vector<TexData> textures;
		std::vector<MatData> materials;
		std::vector<MeshData> meshData;
		std::vector<Mesh const*> preloaded;
		Mesh::Type type = Mesh::Type::eStatic;
		Texture::Space mode = Texture::Space::eSRGBNonLinear;
		Colour tint = colours::white;
		bool bDropColour = false;
	};

	struct LoadRequest final
	{
		stdfs::path jsonID;
		IOReader const* pReader = nullptr;
	};

public:
	static std::string const s_tName;

public:
	static size_t idHash(stdfs::path const& id);
	static size_t strHash(std::string_view id);
	static Info parseOBJ(LoadRequest const& request);

public:
	Model(stdfs::path id, Info info);

protected:
	std::vector<Mesh const*> m_meshes;
	std::deque<Material::Inst> m_materials;
	std::deque<Mesh> m_loadedMeshes;
	std::unordered_map<size_t, Material> m_loadedMaterials;
	std::unordered_map<size_t, Texture> m_loadedTextures;

public:
	std::vector<Mesh const*> meshes() const;

public:
	Status update() override;
};
} // namespace le::gfx
