#include <imgui.h>
#include <spaced/core/enumerate.hpp>
#include <spaced/core/fixed_string.hpp>
#include <spaced/core/visitor.hpp>
#include <spaced/imcpp/inspector.hpp>
#include <spaced/imcpp/reflector.hpp>
#include <spaced/resources/resources.hpp>
#include <spaced/scene/freecam_controller.hpp>
#include <spaced/scene/mesh_animator.hpp>
#include <spaced/scene/mesh_renderer.hpp>
#include <spaced/scene/scene.hpp>

namespace spaced::imcpp {
namespace {
void inspect(OpenWindow w, std::vector<graphics::RenderInstance>& out_instances) {
	if (auto tn = imcpp::TreeNode{"Instances"}) {
		auto const reflector = imcpp::Reflector{w};
		bool unified_scaling{true};
		auto to_remove = std::optional<std::size_t>{};
		for (auto [instance, index] : enumerate(out_instances)) {
			if (auto tn = imcpp::TreeNode{FixedString{"[{}]", index}.c_str()}) {
				reflector(instance.transform, unified_scaling, false);
				reflector(instance.tint, true);
				if (imcpp::small_button_red("X")) { to_remove = index; }
			}
		}
		if (to_remove) { out_instances.erase(out_instances.begin() + static_cast<std::ptrdiff_t>(*to_remove)); }
		if (ImGui::Button("+")) { out_instances.push_back({}); }
	}
}
} // namespace

void Inspector::display(Scene& scene) {
	if (bool show_inspector = static_cast<bool>(target)) {
		auto const width = ImGui::GetMainViewport()->Size.x * width_pct;
		ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x - width, 0.0f});
		ImGui::SetNextWindowSize({width, ImGui::GetIO().DisplaySize.y});
		if (auto w = Window{"Inspector", &show_inspector, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove}) { draw_to(w, scene); }
		if (!show_inspector) { target = {}; }
	}
}

void Inspector::draw_to(NotClosed<Window> w, Scene& scene) {
	auto const visitor = Visitor{
		[](std::monostate) {},
		[&](Id<Entity> id) {
			auto* entity = scene.find_entity(id);
			if (entity == nullptr) { return; }
			ImGui::Text("%s", FixedString{"{}", entity->id().value()}.c_str());
			auto& entity_name = get_entity_name(id, entity->get_node().name);
			if (entity_name("Name")) { entity->get_node().name = entity_name.view(); }
			bool is_active{entity->is_active()};
			if (ImGui::Checkbox("Active", &is_active)) { entity->set_active(is_active); }
			if (auto tn = imcpp::TreeNode("Transform", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
				auto unified_scaling = bool{true};
				imcpp::Reflector{w}(entity->get_transform(), unified_scaling, true);
			}
		},
		[&](Type type) {
			switch (type) {
			case Type::eSceneCamera: {
				imcpp::TreeNode::leaf("Scene Camera", ImGuiTreeNodeFlags_SpanFullWidth);
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

auto Inspector::get_entity_name(Id<Entity> id, std::string_view name) -> InputText<>& {
	if (m_entity_name.previous != id) {
		m_entity_name.previous = id;
		m_entity_name.input_text.set(name);
	}
	return m_entity_name.input_text;
}
} // namespace spaced::imcpp
