#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "core/colour.hpp"
#include "core/flags.hpp"
#include "engine/assets/asset.hpp"
#include "geometry.hpp"

namespace le::gfx
{
class Texture;
class Cubemap;

struct Albedo final
{
	Colour ambient = colours::white;
	Colour diffuse = colours::white;
	Colour specular = colours::white;
};

class Material final : public Asset
{
public:
	enum class Flag : u8
	{
		eTextured,
		eLit,
		eOpaque,
		eDropColour,
		eUI,
		eSkybox,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	struct Inst final
	{
		Material* pMaterial = nullptr;
		Texture* pDiffuse = nullptr;
		Texture* pSpecular = nullptr;
		Cubemap* pCubemap = nullptr;
		Colour tint = colours::white;
		Colour dropColour = colours::black;
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

class Mesh : public Asset
{
public:
	enum class Type : u8
	{
		eStatic,
		eDynamic,
		eCOUNT_
	};

	struct Info final
	{
		Geometry geometry;
		Material::Inst material;
		Type type = Type::eStatic;
	};

public:
	Material::Inst m_material;

protected:
	std::unique_ptr<struct MeshImpl> m_uImpl;
	Type m_type;

public:
	Mesh(stdfs::path id, Info info);
	Mesh(Mesh&&);
	Mesh& operator=(Mesh&&);
	~Mesh() override;

public:
	void updateGeometry(Geometry geometry);

public:
	Status update() override;

private:
	friend class Renderer;
	friend class RendererImpl;
};
} // namespace le::gfx
