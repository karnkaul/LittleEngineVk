#include <imgui.h>
#include <le/core/enumerate.hpp>
#include <le/core/fixed_string.hpp>
#include <le/imcpp/reflector.hpp>
#include <le/scene/collision.hpp>
#include <le/scene/freecam_controller.hpp>
#include <le/scene/imcpp/entity_inspector.hpp>
#include <le/scene/mesh_animator.hpp>
#include <le/scene/mesh_renderer.hpp>
#include <le/scene/shape_renderer.hpp>
#include <le/scene/sound_controller.hpp>

namespace le::imcpp {
namespace {
void inspect_instances(OpenWindow w, std::vector<graphics::RenderInstance>& out_instances) {
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

void inspect_component(OpenWindow w, ShapeRenderer& shape_renderer) {
	auto const reflector = imcpp::Reflector{w};
	auto tint = shape_renderer.get_tint();
	if (reflector(tint, true)) { shape_renderer.set_tint(tint); }
}

void inspect_component(OpenWindow w, MeshRenderer& mesh_renderer) { inspect_instances(w, mesh_renderer.instances); }

void inspect_component(OpenWindow /*w*/, MeshAnimator& animator) {
	auto const animation_id = animator.get_animation_id();
	auto const preview = animation_id ? FixedString{"{}", animation_id->value()} : FixedString{"[None]"};
	auto const animations = animator.get_animations();
	if (auto combo = imcpp::Combo{"Active", preview.c_str()}) {
		if (ImGui::Selectable("[None]")) {
			animator.reset_animation_id();
		} else {
			for (std::size_t i = 0; i < animations.size(); ++i) {
				if (ImGui::Selectable(FixedString{"{}", i}.c_str(), animation_id && i == animation_id->value())) { animator.set_animation_id(i); }
			}
		}
	}

	if (auto const* animation = animator.get_animation()) {
		ImGui::Text("%s", FixedString{"Duration: {:.1f}s", animation->duration().count()}.c_str());
		float const progress = animator.elapsed / animation->duration();
		ImGui::ProgressBar(progress);
	}
}

void inspect_component(OpenWindow w, FreecamController& freecam_controller) {
	Reflector{w}("Move Speed", freecam_controller.move_speed);
	ImGui::DragFloat("Look Speed", &freecam_controller.look_speed);
	auto degrees = freecam_controller.pitch.to_degrees();
	if (ImGui::DragFloat("Pitch", &degrees.value, 0.25f, -89.0f, 89.0f)) { freecam_controller.pitch = degrees; }
	degrees = freecam_controller.yaw.to_degrees();
	if (ImGui::DragFloat("Yaw", &degrees.value, 0.25f, -180.0f, 180.0f)) { freecam_controller.yaw = degrees; }
}

void inspect_component(OpenWindow w, ColliderAabb& collider) { Reflector{w}("Size", collider.aabb_size, 0.25f); }

auto inspect_component(OpenWindow /*w*/, SoundController& controller) {
	auto volume = controller.get_volume().value();
	if (ImGui::SliderFloat("volume", &volume, Volume::min_value_v, Volume::max_value_v)) { controller.set_volume(Volume{volume}); }
}

template <std::derived_from<Component> T>
auto inspect_if(OpenWindow w, Entity& out, char const* label) -> void {
	if (auto* component = out.find_component<T>()) {
		if (auto tn = imcpp::TreeNode{label, ImGuiTreeNodeFlags_Framed}) { inspect_component(w, *component); }
	}
}
} // namespace

auto EntityInspector::inspect(OpenWindow w, Entity& out) -> void {
	ImGui::Text("%s", FixedString{"{}", out.id().value()}.c_str());
	auto& entity_name = get_entity_name(out.id(), out.get_node().name);
	if (entity_name("Name")) { out.get_node().name = entity_name.view(); }
	bool is_active{out.is_active()};
	if (ImGui::Checkbox("Active", &is_active)) { out.set_active(is_active); }
	if (auto tn = imcpp::TreeNode("Transform", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
		auto unified_scaling = bool{true};
		imcpp::Reflector{w}(out.get_transform(), unified_scaling, true);
	}

	inspect_components(w, out);
}

auto EntityInspector::inspect_components(OpenWindow w, Entity& out) -> void {
	inspect_if<ShapeRenderer>(w, out, "ShapeRenderer");
	inspect_if<MeshRenderer>(w, out, "MeshRenderer");
	inspect_if<MeshAnimator>(w, out, "MeshAnimator");
	inspect_if<FreecamController>(w, out, "FreecamController");
	inspect_if<ColliderAabb>(w, out, "ColliderAabb");
	inspect_if<SoundController>(w, out, "SoundController");
}

auto EntityInspector::get_entity_name(Id<Entity> const id, std::string_view const name) -> InputText<>& {
	if (!m_entity_name.previous || *m_entity_name.previous != id) {
		m_entity_name.previous = id;
		m_entity_name.input_text.set(name);
	}
	return m_entity_name.input_text;
}
} // namespace le::imcpp
