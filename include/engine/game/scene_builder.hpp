#pragma once
#include <engine/ecs/registry.hpp>
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

struct SceneBuilder
{
	struct Info final
	{
		gfx::Renderer::View view;
		gfx::Renderer::ClearValues clearValues;
		std::vector<gfx::DirLight> dirLights;
		gfx::Texture const* pSkybox = nullptr;
		gfx::Pipeline const* p3Dpipe = nullptr;
		gfx::Pipeline const* pUIpipe = nullptr;
	};

	Info info;

	virtual gfx::Renderer::Scene build(Registry const& registry) const;
};
} // namespace le
