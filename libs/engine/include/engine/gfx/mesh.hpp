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
	Colour ambient = colours::White;
	Colour diffuse = colours::White;
	Colour specular = colours::White;
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
		Flags flags;
	};

	struct Info final
	{
		Albedo albedo;
	};

public:
	Albedo m_albedo;
	f32 m_shininess = 32.0f;

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
