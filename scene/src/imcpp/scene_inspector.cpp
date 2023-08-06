#include <imgui.h>
#include <le/core/enumerate.hpp>
#include <le/core/fixed_string.hpp>
#include <le/core/visitor.hpp>
#include <le/imcpp/reflector.hpp>
#include <le/resources/resources.hpp>
#include <le/scene/freecam_controller.hpp>
#include <le/scene/imcpp/scene_inspector.hpp>
#include <le/scene/mesh_animator.hpp>
#include <le/scene/mesh_renderer.hpp>
#include <le/scene/scene.hpp>

namespace le::imcpp {
void SceneInspector::display(Scene& scene) {
	if (bool show_inspector = static_cast<bool>(target)) {
		auto const width = ImGui::GetMainViewport()->Size.x * width_pct;
		ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x - width, 0.0f});
		ImGui::SetNextWindowSize({width, ImGui::GetIO().DisplaySize.y});
		if (auto w = Window{"Inspector", &show_inspector, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove}) { draw_to(w, scene); }
		if (!show_inspector) { target = {}; }
	}
}

auto SceneInspector::set_entity_inspector(std::unique_ptr<EntityInspector> entity_inspector) -> void {
	if (!entity_inspector) { return; }
	m_entity_inspector = std::move(entity_inspector);
}

void SceneInspector::draw_to(NotClosed<Window> w, Scene& scene) {
	auto const visitor = Visitor{
		[](std::monostate) {},
		[&](Id<Entity> id) {
			auto* entity = scene.find_entity(id);
			if (entity == nullptr) { return; }
			m_entity_inspector->inspect(w, *entity);
		},
		[&](Type type) {
			switch (type) {
			case Type::eCamera: {
				imcpp::TreeNode::leaf("Camera", ImGuiTreeNodeFlags_SpanFullWidth);
				if (auto tn = imcpp::TreeNode{"Transform", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen}) {
					bool unified_scaling{true};
					imcpp::Reflector{w}(scene.main_camera.transform, unified_scaling, {});
				}
				ImGui::DragFloat("Exposure", &scene.main_camera.exposure, 0.25f, 1.0f, 100.0f);
				std::string_view const type = std::holds_alternative<graphics::Camera::Orthographic>(scene.main_camera.type) ? "Orthographic" : "Perspective";
				if (auto combo = imcpp::Combo{"Type", type.data()}) {
					if (combo.item("Perspective", type == "Perspective")) { scene.main_camera.type = graphics::Camera::Perspective{}; }
					if (combo.item("Orthographic", type == "Orthographic")) { scene.main_camera.type = graphics::Camera::Orthographic{}; }
				}
				std::visit([w](auto& camera) { Reflector{w}(camera); }, scene.main_camera.type);
				break;
			}
			case Type::eLights: {
				imcpp::TreeNode::leaf("Lights", ImGuiTreeNodeFlags_SpanFullWidth);
				auto const inspect_dir_light = [w](graphics::Lights::Directional& dir_light) {
					imcpp::Reflector{w}(dir_light.diffuse, false);
					imcpp::Reflector{w}("Direction", dir_light.direction);
				};
				if (auto tn = imcpp::TreeNode{"Directional", ImGuiTreeNodeFlags_Framed}) {
					auto to_remove = std::optional<std::size_t>{};
					for (auto [dir_light, index] : enumerate(scene.lights.directional)) {
						if (auto tn = TreeNode{FixedString{"[{}]", index}.c_str()}) {
							inspect_dir_light(dir_light);
							if (small_button_red("X")) { to_remove = index; }
						}
					}
					if (to_remove) { scene.lights.directional.erase(scene.lights.directional.begin() + static_cast<std::ptrdiff_t>(*to_remove)); }
					ImGui::Separator();
					if (ImGui::Button("Add")) { scene.lights.directional.push_back({}); }
				}
				break;
			}
			}
		},
	};

	std::visit(visitor, target.payload);
}
} // namespace le::imcpp
