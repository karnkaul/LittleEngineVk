#pragma once
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
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	std::unique_ptr<gfx::Text2D> uText;
	Flags flags;

	void setText(gfx::Text2D::Info info);
	void reset(Flags toReset);

	std::vector<gfx::Mesh const*> meshes() const;
};

class SceneBuilder
{
public:
	struct Info final
	{
		std::vector<gfx::DirLight> dirLights;
		stdfs::path skyboxCubemapID;
		glm::vec2 uiSpace = {0.0f, 0.0f};
		gfx::Camera const* pCamera = nullptr;
		gfx::Pipeline const* p3Dpipe = nullptr;
		gfx::Pipeline const* pUIpipe = nullptr;
		bool bDynamicUI = true;
		bool bClampUIViewport = false;
		Colour clearColour = colours::black;
	};

protected:
	Info m_info;

public:
	SceneBuilder(gfx::Camera const& camera);
	SceneBuilder(Info info);

public:
	static f32 framebufferAspect();
	static glm::vec2 uiSpace(glm::vec2 const& uiSpace);

public:
	virtual gfx::Renderer::Scene build(Registry const& registry) const;
};
} // namespace le
