#pragma once
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>
#include "engine/assets/asset.hpp"
#include "engine/gfx/geometry.hpp"
#include "engine/gfx/mesh.hpp"
#include "engine/gfx/texture.hpp"

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
	};
	struct MeshData final
	{
		Geometry geometry;
		std::string id;
		std::vector<size_t> materialIndices;
		f32 shininess = 32.0f;
	};

	struct Info final
	{
		stdfs::path id;
		std::vector<TexData> textures;
		std::vector<MatData> materials;
		std::vector<MeshData> meshData;
		std::vector<Mesh const*> preloaded;
		Mesh::Type type = Mesh::Type::eStatic;
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
	static Info parseOBJ(LoadRequest const& request);

public:
	Model(stdfs::path id, Info info);

protected:
	std::deque<Mesh const*> m_meshes;
	std::deque<Material::Inst> m_materials;
	std::deque<Mesh> m_loadedMeshes;
	std::unordered_map<std::string, Material> m_loadedMaterials;
	std::unordered_map<std::string, Texture> m_loadedTextures;

public:
	size_t meshCount() const;
	void fillMeshes(std::vector<Mesh const*>& out_meshes) const;

public:
	Status update() override;
};
} // namespace le::gfx
