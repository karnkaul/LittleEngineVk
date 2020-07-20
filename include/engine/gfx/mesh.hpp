#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <core/flags.hpp>
#include <engine/assets/asset.hpp>
#include <engine/gfx/geometry.hpp>
#include <engine/resources/resource_types.hpp>

namespace le::gfx
{
class Mesh : public Asset
{
public:
	enum class Type : s8
	{
		eStatic,
		eDynamic,
		eCOUNT_
	};

	struct Info final
	{
		Geometry geometry;
		res::Material::Inst material;
		Type type = Type::eStatic;
	};

public:
	res::Material::Inst m_material;
	u64 m_triCount = 0;

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
