#include <core/transform.hpp>
#include <engine/assets/resources.hpp>
#include <engine/gfx/model.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/levk.hpp>

namespace le
{
void UIComponent::setText(gfx::Text2D::Info info)
{
	uText = std::make_unique<gfx::Text2D>();
	uText->setup(std::move(info));
	flags.set(Flag::eText);
}

void UIComponent::reset(Flags toReset)
{
	auto resetPtr = [&toReset](Flag flag, auto& m) {
		if (toReset.isSet(flag))
		{
			m.reset();
		}
	};
	resetPtr(Flag::eText, uText);
}

std::vector<gfx::Mesh const*> UIComponent::meshes() const
{
	std::vector<gfx::Mesh const*> ret;
	if (flags.isSet(Flag::eText) && uText && uText->isReady())
	{
		ret.push_back(uText->mesh());
	}
	return ret;
}

SceneBuilder::SceneBuilder(gfx::Camera const& camera)
{
	m_info.pCamera = &camera;
}

SceneBuilder::SceneBuilder(Info info) : m_info(std::move(info))
{
	ASSERT(m_info.pCamera, "Camera cannot be null!");
}

f32 SceneBuilder::framebufferAspect()
{
	auto const ifbSize = engine::framebufferSize();
	return ifbSize.x == 0 || ifbSize.y == 0 ? 1.0f : (f32)ifbSize.x / (f32)ifbSize.y;
}

glm::vec2 SceneBuilder::uiSpace(glm::vec2 const& uiSpace)
{
	auto const ifbSize = engine::framebufferSize();
	glm::vec2 const fbSize = {(f32)ifbSize.x, (f32)ifbSize.y};
	f32 const uiAspect = uiSpace.x / uiSpace.y;
	f32 const fbAspect = fbSize.x / fbSize.y;
	f32 const x = uiAspect > fbAspect ? uiSpace.x : uiSpace.x * (fbAspect / uiAspect);
	f32 const y = fbAspect > uiAspect ? uiSpace.y : uiSpace.y * (uiAspect / fbAspect);
	return {x, y};
}

gfx::Renderer::Scene SceneBuilder::build(Registry const& registry) const
{
	ASSERT(m_info.pCamera, "Camera is null!");
	gfx::Renderer::Scene scene;
	scene.clear = {{1.0f, 0.0f}, m_info.clearColour};
	scene.dirLights = m_info.dirLights;
	scene.view.pos_v = m_info.pCamera->m_position;
	scene.view.mat_v = m_info.pCamera->view();
	auto const ifbSize = engine::framebufferSize();
	glm::vec2 const fbSize = {(f32)ifbSize.x, (f32)ifbSize.y};
	f32 const fbAspect = fbSize.x == 0.0f || fbSize.y == 0.0f ? 1.0f : fbSize.x / fbSize.y;
	auto const uiSp = m_info.uiSpace.x == 0.0f || m_info.uiSpace.y == 0.0f ? fbSize : m_info.uiSpace;
	auto const ui = m_info.bDynamicUI ? fbSize : uiSpace(uiSp);
	scene.view.mat_p = m_info.pCamera->perspective(fbAspect);
	scene.view.mat_vp = scene.view.mat_p * scene.view.mat_v;
	scene.view.mat_ui = m_info.pCamera->ui({ui, 2.0f});
	if (!m_info.skyboxCubemapID.empty())
	{
		auto pCubemap = Resources::inst().get<gfx::Texture>(m_info.skyboxCubemapID);
		if (pCubemap && pCubemap->isReady())
		{
			scene.view.skybox.pCubemap = pCubemap;
		}
	}
	gfx::Renderer::Batch batch3D;
	gfx::Renderer::Batch batchUI;
	{
		auto view = registry.view<Transform, TAsset<gfx::Model>>();
		for (auto& [entity, query] : view)
		{
			auto& [pTransform, cModel] = query;
			if (auto pModel = cModel->get(); pModel && pModel->isReady())
			{
				batch3D.drawables.push_back({pModel->meshes(), pTransform, m_info.p3Dpipe});
			}
		}
	}
	{
		auto view = registry.view<Transform, TAsset<gfx::Mesh>>();
		for (auto& [entity, query] : view)
		{
			auto& [pTransform, cMesh] = query;
			if (auto pMesh = cMesh->get(); pMesh && pMesh->isReady())
			{
				batch3D.drawables.push_back({{pMesh}, pTransform, m_info.p3Dpipe});
			}
		}
	}
	{
		auto view = registry.view<UIComponent>();
		for (auto& [entity, query] : view)
		{
			auto& [pUI] = query;
			auto meshes = pUI->meshes();
			if (!meshes.empty())
			{
				auto pTransform = registry.component<Transform>(entity);
				batchUI.drawables.push_back({pUI->meshes(), pTransform ? pTransform : &Transform::s_identity, m_info.pUIpipe});
			}
		}
	}
	if (!batch3D.drawables.empty())
	{
		scene.batches.push_back(std::move(batch3D));
	}
	if (!batchUI.drawables.empty())
	{
		if (m_info.bClampUIViewport)
		{
			batchUI.scissor = gfx::ScreenRect::sizeCentre({uiSp.x / ui.x, uiSp.y / ui.y});
		}
		scene.batches.push_back(std::move(batchUI));
	}
	return scene;
}
} // namespace le
