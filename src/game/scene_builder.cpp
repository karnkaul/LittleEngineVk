#include <core/transform.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/levk.hpp>
#include <engine/resources/resources.hpp>

namespace le {
using namespace ecs;

Text2D& UIComponent::setText(Text2D::Info info) {
	text.setup(std::move(info));
	flags.set(Flag::eText);
	return text;
}

Text2D& UIComponent::setText(res::Font::Text data) {
	if (text.mesh().status() == res::Status::eIdle) {
		auto font = res::find<res::Font>("fonts/default");
		if (font) {
			Text2D::Info info;
			info.data = std::move(data);
			info.font = *font;
			info.id = (id.empty() ? "(ui)" : id) / "text";
			setText(std::move(info));
		}
	} else {
		text.updateText(std::move(data));
	}
	return text;
}

Text2D& UIComponent::setText(std::string text) {
	if (this->text.mesh().status() == res::Status::eIdle) {
		res::Font::Text data;
		data.text = std::move(text);
		setText(std::move(data));
	} else {
		this->text.updateText(std::move(text));
	}
	return this->text;
}

res::Mesh UIComponent::setQuad(glm::vec2 const& size, glm::vec2 const& pivot) {
	gfx::Geometry geometry = gfx::createQuad(size, pivot);
	if (mesh.resource.status() == res::Status::eIdle) {
		res::Mesh::CreateInfo info;
		info.geometry = std::move(geometry);
		info.type = res::Mesh::Type::eDynamic;
		info.material.flags.set(res::Material::Flag::eUI);
		mesh = res::load((id.empty() ? "(ui)" : id) / "quad", std::move(info));
	} else {
		mesh.resource.updateGeometry(std::move(geometry));
	}
	flags.set(Flag::eMesh);
	return mesh;
}

void UIComponent::reset(Flags toReset) {
	if (toReset.test(Flag::eMesh)) {
		mesh = {};
	}
	if (toReset.test(Flag::eText)) {
		text = {};
	}
}

std::vector<res::Mesh> UIComponent::meshes() const {
	std::vector<res::Mesh> ret;
	if (flags.test(Flag::eText) && text.ready()) {
		ret.push_back(text.mesh());
	}
	if (flags.test(Flag::eMesh) && mesh.resource.status() == res::Status::eReady) {
		ret.push_back(mesh.resource);
	}
	return ret;
}

SceneBuilder::~SceneBuilder() = default;

gfx::render::Driver::Scene SceneBuilder::build(gfx::Camera const& camera, Registry const& registry) const {
	gfx::render::Driver::Scene scene;
	gfx::render::Driver::Batch batch3D;
	std::vector<gfx::render::Driver::Batch> batchUI;
	gfx::Pipeline pipe3D, pipeUI;
	std::optional<f32> orthoDepth;
	GameScene::Desc::Flags flags;
	{
		auto view = registry.view<GameScene::Desc>();
		if (!view.empty()) {
			auto& [_, query] = *view.begin();
			auto const [desc] = query;
			scene.clear = {desc.clearDepth, desc.clearColour};
			scene.dirLights = desc.dirLights;
			if (!desc.skyboxCubemapID.empty()) {
				auto skybox = res::find<res::Texture>(desc.skyboxCubemapID);
				if (skybox) {
					scene.view.skybox.cubemap = *skybox;
				}
			}
			pipe3D = desc.pipe3D;
			pipeUI = desc.pipeUI;
			flags = desc.flags;
			orthoDepth = desc.orthoDepth;
		}
	}
	if (auto pWindow = engine::mainWindow()) {
		pWindow->driver().fill(scene.view, engine::viewport(), camera, orthoDepth.value_or(2.0f));
	}
	{
		auto view = registry.view<Transform, res::Model>();
		for (auto& [entity, query] : view) {
			if (auto& [transform, model] = query; model.status() == res::Status::eReady) {
				batch3D.drawables.push_back({model.meshes(), transform, pipe3D});
			}
		}
	}
	{
		auto view = registry.view<Transform, res::Mesh>();
		for (auto& [entity, query] : view) {
			if (auto& [transform, mesh] = query; mesh.status() == res::Status::eReady) {
				batch3D.drawables.push_back({{mesh}, transform, pipe3D});
			}
		}
	}
	{
		auto view = registry.view<UIComponent>();
		for (auto& [entity, query] : view) {
			auto const [ui] = query;
			auto meshes = ui.meshes();
			if (!meshes.empty()) {
				auto pTransform = registry.find<Transform>(entity);
				batchUI.push_back({});
				auto& batch = batchUI.back();
				batch.scissor = ui.scissor;
				batch.bIgnoreGameView = ui.bIgnoreGameView;
				batch.drawables.push_back({std::move(meshes), pTransform ? *pTransform : Transform::s_identity, pipeUI});
			}
		}
	}
	if (!batch3D.drawables.empty()) {
		scene.batches.push_back(std::move(batch3D));
	}
	if (!batchUI.empty()) {
		std::move(batchUI.begin(), batchUI.end(), std::back_inserter(scene.batches));
	}
	return scene;
}
} // namespace le
