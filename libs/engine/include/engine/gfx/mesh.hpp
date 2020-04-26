#pragma once
#include <filesystem>
#include <memory>
#include <glm/glm.hpp>
#include "core/colour.hpp"
#include "core/flags.hpp"
#include "engine/assets/asset.hpp"
#include "geometry.hpp"

namespace stdfs = std::filesystem;

namespace le::gfx
{
class Texture;

struct Albedo final
{
	glm::vec3 diffuse = glm::vec3(1.0f);
	glm::vec3 ambient = glm::vec3(1.0f);
	glm::vec3 specular = glm::vec3(1.0f);
};

class Material final : public Asset
{
public:
	enum class Flag : u8
	{
		eTextured = 0,
		eLit,
		eCOUNT_
	};

	using Flags = TFlags<Flag>;

	struct Inst final
	{
		Material* pMaterial = nullptr;
		Texture* pDiffuse = nullptr;
		Texture* pSpecular = nullptr;
		Colour tint = colours::White;
	};

	struct Info final
	{
		Albedo albedo;
		Flags flags;
	};

public:
	Albedo m_albedo;
	Flags m_flags;
	f32 shininess = 32.0f;

public:
	Material(stdfs::path id, Info info);

public:
	Status update() override;
};

class Mesh final : public Asset
{
public:
	struct Info final
	{
		Geometry geometry;
		Material::Inst material;
	};

public:
	Material::Inst m_material;
	std::unique_ptr<struct MeshImpl> m_uImpl;

public:
	Mesh(stdfs::path id, Info info);
	~Mesh() override;

public:
	Status update() override;
};
} // namespace le::gfx
