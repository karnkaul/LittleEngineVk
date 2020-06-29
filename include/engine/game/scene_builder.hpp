#pragma once
#include <core/flags.hpp>
#include <engine/ecs/registry.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/gfx/font.hpp>
#include <engine/gfx/light.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/gfx/renderer.hpp>
#include <engine/gfx/texture.hpp>

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
	std::unique_ptr<gfx::Text2D> uText;
	std::unique_ptr<gfx::Mesh> uMesh;
	Flags flags;

	gfx::Text2D& setText(gfx::Text2D::Info info);
	gfx::Mesh& setQuad(glm::vec2 const& size, glm::vec2 const& pivot = {});
	void reset(Flags toReset);

	std::vector<gfx::Mesh const*> meshes() const;
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
		gfx::Camera const* pCamera = nullptr;
		gfx::Pipeline const* p3Dpipe = nullptr;
		gfx::Pipeline const* pUIpipe = nullptr;
		glm::vec2 clearDepth = {1.0f, 0.0f};
		Colour clearColour = colours::black;
		Flags flags = Flag::eDynamicUI;
	};

protected:
	Info m_info;

public:
	SceneBuilder(gfx::Camera const& camera);
	SceneBuilder(Info info);

public:
	static f32 framebufferAspect();
	static glm::vec3 uiProjection(glm::vec3 const& uiSpace, glm::ivec2 const& framebuffer);
	static glm::vec3 uiProjection(glm::vec3 const& uiSpace);

public:
	virtual gfx::Renderer::Scene build(Registry const& registry) const;
};
} // namespace le
