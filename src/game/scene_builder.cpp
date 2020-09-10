#include <core/transform.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/resources/resources.hpp>
#include <engine/levk.hpp>
#include <levk_impl.hpp>

namespace le
{
Text2D& UIComponent::setText(Text2D::Info info)
{
	text.setup(std::move(info));
	flags.set(Flag::eText);
	return text;
}

Text2D& UIComponent::setText(res::Font::Text data)
{
	if (text.mesh().status() == res::Status::eIdle)
	{
		auto font = res::find<res::Font>("fonts/default");
		if (font)
		{
			Text2D::Info info;
			info.data = std::move(data);
			info.font = *font;
			info.id = (id.empty() ? "(ui)" : id) / "text";
			setText(std::move(info));
		}
	}
	else
	{
		text.updateText(std::move(data));
	}
	return text;
}

Text2D& UIComponent::setText(std::string text)
{
	if (this->text.mesh().status() == res::Status::eIdle)
	{
		res::Font::Text data;
		data.text = std::move(text);
		setText(std::move(data));
	}
	else
	{
		this->text.updateText(std::move(text));
	}
	return this->text;
}

res::Mesh UIComponent::setQuad(glm::vec2 const& size, glm::vec2 const& pivot)
{
	gfx::Geometry geometry = gfx::createQuad(size, pivot);
	if (mesh.resource.status() == res::Status::eIdle)
	{
		res::Mesh::CreateInfo info;
		info.geometry = std::move(geometry);
		info.type = res::Mesh::Type::eDynamic;
		info.material.flags.set(res::Material::Flag::eUI);
		mesh = res::load((id.empty() ? "(ui)" : id) / "quad", std::move(info));
	}
	else
	{
		mesh.resource.updateGeometry(std::move(geometry));
	}
	flags.set(Flag::eMesh);
	return mesh;
}

void UIComponent::reset(Flags toReset)
{
	if (toReset.test(Flag::eMesh))
	{
		mesh = {};
	}
	if (toReset.test(Flag::eText))
	{
		text = {};
	}
}

std::vector<res::Mesh> UIComponent::meshes() const
{
	std::vector<res::Mesh> ret;
	if (flags.test(Flag::eText) && text.ready())
	{
		ret.push_back(text.mesh());
	}
	if (flags.test(Flag::eMesh) && mesh.resource.status() == res::Status::eReady)
	{
		ret.push_back(mesh.resource);
	}
	return ret;
}

SceneBuilder::~SceneBuilder() = default;

glm::vec3 SceneBuilder::uiProjection(glm::vec3 const& uiSpace, glm::ivec2 const& renderArea)
{
	f32 const uiX = uiSpace.x == 0.0f ? 1.0f : uiSpace.x;
	f32 const uiY = uiSpace.y == 0.0f ? 1.0f : uiSpace.y;
	f32 const uiAspect = uiX / uiY;
	f32 const fbAspect = (f32)(renderArea.x == 0 ? 1 : renderArea.x) / (f32)(renderArea.y == 0 ? 1 : renderArea.y);
	f32 const x = uiAspect > fbAspect ? uiX : uiX * (fbAspect / uiAspect);
	f32 const y = fbAspect > uiAspect ? uiY : uiY * (uiAspect / fbAspect);
	return {x, y, uiSpace.z};
}

glm::vec3 SceneBuilder::uiProjection(glm::vec3 const& uiSpace)
{
	auto const ifbSize = engine::framebufferSize();
	auto const gameRect = engine::gameRectSize();
	glm::ivec2 const renderArea = {(s32)((f32)ifbSize.x * gameRect.x), (s32)((f32)ifbSize.y * gameRect.y)};
	return uiProjection(uiSpace, renderArea);
}

gfx::Renderer::Scene SceneBuilder::build(gfx::Camera const& camera, Registry const& registry) const
{
	auto const ifbSize = engine::framebufferSize();
	auto const gameRect = engine::gameRectSize();
	glm::vec2 const fbSize = {ifbSize.x == 0 ? 1.0f : (f32)ifbSize.x, ifbSize.y == 0 ? 1.0f : (f32)ifbSize.y};
	glm::vec2 const fRenderArea = fbSize * gameRect;
	glm::ivec2 const iRenderArea = {(s32)fRenderArea.x, (s32)fRenderArea.y};
	gfx::Renderer::Scene scene;
	scene.view.pos_v = camera.position;
	scene.view.mat_v = camera.view();
	scene.view.mat_p = camera.perspective(fRenderArea.x / fRenderArea.y);
	scene.view.mat_vp = scene.view.mat_p * scene.view.mat_v;
	scene.view.mat_ui = camera.ui(engine::g_uiSpace);
	gfx::Renderer::Batch batch3D;
	gfx::Renderer::Batch batchUI;
	gfx::Pipeline pipe3D, pipeUI;
	glm::vec3 uiSpace = glm::vec3(fRenderArea, 2.0f);
	SceneDesc::Flags flags;
	{
		auto view = registry.view<SceneDesc>();
		if (!view.empty())
		{
			auto& [_, query] = *view.begin();
			auto& [pDesc] = query;
			scene.clear = {pDesc->clearDepth, pDesc->clearColour};
			scene.dirLights = pDesc->dirLights;
			if (!pDesc->skyboxCubemapID.empty())
			{
				auto skybox = res::find<res::Texture>(pDesc->skyboxCubemapID);
				if (skybox)
				{
					scene.view.skybox.cubemap = *skybox;
				}
			}
			pipe3D = pDesc->pipe3D;
			pipeUI = pDesc->pipeUI;
			flags = pDesc->flags;
			if (pDesc->uiSpace.x > 0.0f && pDesc->uiSpace.y > 0.0f)
			{
				uiSpace = pDesc->uiSpace;
			}
		}
	}
	engine::g_uiSpace = flags.test(SceneDesc::Flag::eDynamicUI) ? glm::vec3(fRenderArea, uiSpace.z) : uiProjection(uiSpace, iRenderArea);
	{
		auto view = registry.view<Transform, res::Model>();
		for (auto& [entity, query] : view)
		{
			if (auto& [pTransform, pModel] = query; pModel->status() == res::Status::eReady)
			{
				batch3D.drawables.push_back({pModel->meshes(), *pTransform, pipe3D});
			}
		}
	}
	{
		auto view = registry.view<Transform, res::Mesh>();
		for (auto& [entity, query] : view)
		{
			if (auto& [pTransform, pMesh] = query; pMesh->status() == res::Status::eReady)
			{
				batch3D.drawables.push_back({{*pMesh}, *pTransform, pipe3D});
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
				batchUI.drawables.push_back({pUI->meshes(), pTransform ? *pTransform : Transform::s_identity, pipeUI});
			}
		}
	}
	if (!batch3D.drawables.empty())
	{
		scene.batches.push_back(std::move(batch3D));
	}
	if (!batchUI.drawables.empty())
	{
		if (!flags.test(SceneDesc::Flag::eDynamicUI) && flags.test(SceneDesc::Flag::eScissoredUI))
		{
			batchUI.scissor = gfx::ScreenRect::sizeCentre({uiSpace.x / engine::g_uiSpace.x, uiSpace.y / engine::g_uiSpace.y});
		}
		scene.batches.push_back(std::move(batchUI));
	}
	return scene;
}
} // namespace le
