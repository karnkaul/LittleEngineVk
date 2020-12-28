#pragma once
#include <core/ec_registry.hpp>
#include <engine/game/state.hpp>
#include <engine/game/text2d.hpp>
#include <engine/gfx/render_driver.hpp>
#include <engine/gfx/viewport.hpp>
#include <engine/resources/resource_types.hpp>
#include <kt/enum_flags/enum_flags.hpp>

namespace le {
struct UIComponent final {
	enum class Flag { eText, eMesh, eCOUNT_ };
	using Flags = kt::enum_flags<Flag>;

	io::Path id;
	Text2D text;
	res::TScoped<res::Mesh> mesh;
	gfx::ScreenRect scissor;
	Flags flags;
	bool bIgnoreGameView = false;

	Text2D& setText(Text2D::Info info);
	Text2D& setText(res::Font::Text data);
	Text2D& setText(std::string text);
	res::Mesh setQuad(glm::vec2 const& size, glm::vec2 const& pivot = {});
	void reset(Flags toReset);

	std::vector<res::Mesh> meshes() const;
};

class SceneBuilder {
  public:
	virtual ~SceneBuilder();

  public:
	virtual gfx::render::Driver::Scene build(gfx::Camera const& camera, ecs::Registry const& registry) const;
};
} // namespace le
