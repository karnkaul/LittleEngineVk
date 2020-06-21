#include <core/transform.hpp>
#include <engine/assets/resources.hpp>
#include <engine/gfx/model.hpp>
#include <engine/game/scene_builder.hpp>

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

gfx::Renderer::Scene SceneBuilder::build(Registry const& registry) const
{
	gfx::Renderer::Scene scene;
	scene.dirLights = info.dirLights;
	scene.clear = info.clearValues;
	scene.view = info.view;
	if (info.pSkybox && info.pSkybox->isReady())
	{
		scene.view.skybox.pCubemap = info.pSkybox;
	}
	gfx::Renderer::Batch batch;
	{
		auto view = registry.view<Transform, TAsset<gfx::Model>>();
		for (auto& [entity, query] : view)
		{
			auto& [pTransform, cModel] = query;
			if (auto pModel = cModel->get(); pModel && pModel->isReady())
			{
				batch.drawables.push_back({pModel->meshes(), pTransform, info.p3Dpipe});
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
				batch.drawables.push_back({{pMesh}, pTransform, info.p3Dpipe});
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
				batch.drawables.push_back({pUI->meshes(), pTransform ? pTransform : &Transform::s_identity, info.pUIpipe});
			}
		}
	}
	scene.batches.push_back(std::move(batch));
	return scene;
}
} // namespace le
