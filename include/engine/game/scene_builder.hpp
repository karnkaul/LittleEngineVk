#pragma once
#include <core/flags.hpp>
#include <core/ecs_registry.hpp>
#include <engine/game/text2d.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/gfx/light.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/gfx/renderer.hpp>
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
	res::Scoped<res::Mesh> mesh;
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
	enum class Flag : s8
	{
		///
		/// \brief UI follows screen size and aspect ratio
		///
		eDynamicUI,
		///
		/// \brief UI clipped to match its aspect ratio
		///
		eScissoredUI,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

public:
	struct Info final
	{
		std::vector<gfx::DirLight> dirLights;
		stdfs::path skyboxCubemapID;
		///
		/// \brief UI Transformation Space (z is depth)
		///
		glm::vec3 uiSpace = {0.0f, 0.0f, 2.0f};
		gfx::Pipeline pipe3D;
		gfx::Pipeline pipeUI;
		glm::vec2 clearDepth = {1.0f, 0.0f};
		Colour clearColour = colours::black;
		Flags flags = Flag::eDynamicUI;
	};

public:
	Info m_info;

public:
	SceneBuilder();
	SceneBuilder(Info info);
	virtual ~SceneBuilder();

public:
	static glm::vec3 uiProjection(glm::vec3 const& uiSpace, glm::ivec2 const& renderArea);
	static glm::vec3 uiProjection(glm::vec3 const& uiSpace);

public:
	virtual gfx::Renderer::Scene build(gfx::Camera const& camera, Registry const& registry) const;
};
} // namespace le
