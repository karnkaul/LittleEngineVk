#include <core/transform.hpp>
#include <engine/assets/resources.hpp>
#include <engine/gfx/model.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/resources/resources.hpp>
#include <engine/levk.hpp>
#include <levk_impl.hpp>

namespace le
{
UIComponent::~UIComponent()
{
	res::unload(mesh);
}

gfx::Text2D& UIComponent::setText(gfx::Text2D::Info info)
{
	uText = std::make_unique<gfx::Text2D>();
	uText->setup(std::move(info));
	flags.set(Flag::eText);
	return *uText;
}

res::Mesh UIComponent::setQuad(glm::vec2 const& size, glm::vec2 const& pivot)
{
	gfx::Geometry geometry = gfx::createQuad(size, pivot);
	if (mesh.status() == res::Status::eIdle)
	{
		res::Mesh::CreateInfo info;
		info.geometry = std::move(geometry);
		info.type = res::Mesh::Type::eDynamic;
		info.material.flags.set(res::Material::Flag::eUI);
		stdfs::path meshID = id.empty() ? "(ui)" : id;
		meshID += "_quad";
		mesh = res::load(meshID, std::move(info));
	}
	else
	{
		mesh.updateGeometry(std::move(geometry));
	}
	flags.set(Flag::eMesh);
	return mesh;
}

void UIComponent::reset(Flags toReset)
{
	auto resetPtr = [&toReset](Flag flag, auto& m) {
		if (toReset.isSet(flag))
		{
			m = nullptr;
		}
	};
	resetPtr(Flag::eText, uText);
	if (toReset.isSet(Flag::eText))
	{
		res::unload(mesh);
		mesh = {};
	}
}

std::vector<res::Mesh> UIComponent::meshes() const
{
	std::vector<res::Mesh> ret;
	if (flags.isSet(Flag::eText) && uText && uText->isReady())
	{
		ret.push_back(uText->mesh());
	}
	if (flags.isSet(Flag::eMesh) && mesh.status() == res::Status::eReady)
	{
		ret.push_back(mesh);
	}
	return ret;
}

SceneBuilder::SceneBuilder() = default;

SceneBuilder::SceneBuilder(Info info) : m_info(std::move(info)) {}

f32 SceneBuilder::framebufferAspect()
{
	auto const ifbSize = engine::framebufferSize();
	return ifbSize.x == 0 || ifbSize.y == 0 ? 1.0f : (f32)ifbSize.x / (f32)ifbSize.y;
}

glm::vec3 SceneBuilder::uiProjection(glm::vec3 const& uiSpace, glm::ivec2 const& framebuffer)
{
	f32 const uiX = uiSpace.x == 0.0f ? 1.0f : uiSpace.x;
	f32 const uiY = uiSpace.y == 0.0f ? 1.0f : uiSpace.y;
	f32 const uiAspect = uiX / uiY;
	f32 const fbAspect = (f32)(framebuffer.x == 0 ? 1 : framebuffer.x) / (f32)(framebuffer.y == 0 ? 1 : framebuffer.y);
	f32 const x = uiAspect > fbAspect ? uiX : uiX * (fbAspect / uiAspect);
	f32 const y = fbAspect > uiAspect ? uiY : uiY * (uiAspect / fbAspect);
	return {x, y, uiSpace.z};
}

glm::vec3 SceneBuilder::uiProjection(glm::vec3 const& uiSpace)
{
	auto const ifbSize = engine::framebufferSize();
	return uiProjection(uiSpace, {ifbSize.x == 0 ? 1 : ifbSize.x, ifbSize.y == 0 ? 1 : ifbSize.y});
}

gfx::Renderer::Scene SceneBuilder::build(gfx::Camera const& camera, Registry const& registry) const
{
	auto const ifbSize = engine::framebufferSize();
	glm::vec2 const fbSize = {ifbSize.x == 0 ? 1.0f : (f32)ifbSize.x, ifbSize.y == 0 ? 1.0f : (f32)ifbSize.y};
	auto const uiSpace = m_info.uiSpace.x == 0.0f || m_info.uiSpace.y == 0.0f ? glm::vec3(fbSize, m_info.uiSpace.z) : m_info.uiSpace;
	engine::g_uiSpace = m_info.flags.isSet(Flag::eDynamicUI) ? glm::vec3(fbSize, m_info.uiSpace.z) : uiProjection(uiSpace, ifbSize);
	gfx::Renderer::Scene scene;
	scene.clear = {m_info.clearDepth, m_info.clearColour};
	scene.dirLights = m_info.dirLights;
	scene.view.pos_v = camera.m_position;
	scene.view.mat_v = camera.view();
	scene.view.mat_p = camera.perspective(fbSize.x / fbSize.y);
	scene.view.mat_vp = scene.view.mat_p * scene.view.mat_v;
	scene.view.mat_ui = camera.ui(engine::g_uiSpace);
	if (!m_info.skyboxCubemapID.empty())
	{
		auto [cubemap, bCubemap] = res::findTexture(m_info.skyboxCubemapID);
		if (bCubemap && cubemap.status() == res::Status::eReady)
		{
			scene.view.skybox.cubemap = cubemap;
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
		auto view = registry.view<Transform, res::Mesh>();
		for (auto& [entity, query] : view)
		{
			auto& [pTransform, pMesh] = query;
			if (pMesh->status() == res::Status::eReady)
			{
				batch3D.drawables.push_back({{*pMesh}, pTransform, m_info.p3Dpipe});
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
		if (!m_info.flags.isSet(Flag::eDynamicUI) && m_info.flags.isSet(Flag::eScissoredUI))
		{
			batchUI.scissor = gfx::ScreenRect::sizeCentre({uiSpace.x / engine::g_uiSpace.x, uiSpace.y / engine::g_uiSpace.y});
		}
		scene.batches.push_back(std::move(batchUI));
	}
	return scene;
}
} // namespace le
