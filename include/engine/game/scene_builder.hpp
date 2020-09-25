#pragma once
#include <core/flags.hpp>
#include <core/ecs_registry.hpp>
#include <engine/game/text2d.hpp>
#include <engine/game/state.hpp>
#include <engine/gfx/render_driver.hpp>
#include <engine/resources/resource_types.hpp>

namespace le
{
struct UIComponent final
{
	enum class Flag
	{
		eText,
		eMesh,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	stdfs::path id;
	Text2D text;
	res::TScoped<res::Mesh> mesh;
	Flags flags;

	Text2D& setText(Text2D::Info info);
	Text2D& setText(res::Font::Text data);
	Text2D& setText(std::string text);
	res::Mesh setQuad(glm::vec2 const& size, glm::vec2 const& pivot = {});
	void reset(Flags toReset);

	std::vector<res::Mesh> meshes() const;
};

class SceneBuilder
{
public:
	virtual ~SceneBuilder();

public:
	static glm::vec3 uiProjection(glm::vec3 const& uiSpace, glm::ivec2 const& renderArea);
	static glm::vec3 uiProjection(glm::vec3 const& uiSpace);

public:
	virtual gfx::render::Driver::Scene build(gfx::Camera const& camera, Registry const& registry) const;
};
} // namespace le
