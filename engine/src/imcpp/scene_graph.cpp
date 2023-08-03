#include <imgui.h>
#include <spaced/core/fixed_string.hpp>
#include <spaced/imcpp/reflector.hpp>
#include <spaced/imcpp/scene_graph.hpp>
#include <spaced/scene/scene.hpp>
#include <spaced/scene/scene_manager.hpp>

namespace spaced::imcpp {
bool SceneGraph::check_stale() {
	bool ret = false;
	if (m_scene != m_prev) {
		m_prev = m_scene;
		m_inspector.target = {};
		ret = true;
	}
	if (auto const* id = std::get_if<EntityId>(&m_inspector.target.payload); id != nullptr && !m_scene->has_entity(*id)) {
		m_inspector.target = {};
		ret = true;
	}
	return ret;
}

Inspector::Target SceneGraph::draw_to(NotClosed<Window> w, Scene& scene) {
	m_scene = &scene;
	check_stale();

	if (ImGui::SliderFloat("Inspector Width", &m_inspector.width_pct, 0.1f, 0.5f, "%.3f")) {
		m_inspector.width_pct = std::clamp(m_inspector.width_pct, 0.1f, 0.5f);
	}

	ImGui::DragFloat("Draw colliders", &scene.collision.draw_line_width);

	ImGui::Separator();
	if (ImGui::Button("Spawn")) { Popup::open("scene_graph.spawn_entity"); }

	ImGui::Separator();
	standalone_node("SceneCamera", Inspector::Type::eSceneCamera);
	standalone_node("Lights", Inspector::Type::eLights);
	draw_scene_tree(w);
	handle_popups();

	m_inspector.display(scene);

	return m_inspector.target;
}

void SceneGraph::standalone_node(char const* label, Inspector::Type type) {
	auto flags = int{};
	flags |= ImGuiTreeNodeFlags_SpanFullWidth;
	if (auto* held_type = std::get_if<Inspector::Type>(&m_inspector.target.payload); held_type != nullptr && *held_type == type) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (imcpp::TreeNode::leaf(label, flags)) { m_inspector.target.payload = type; }
	if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
		m_right_clicked_target.payload = type;
		Popup::open("scene_graph.right_click");
	}
}

bool SceneGraph::walk_node(Node& node) {
	auto node_locator = m_scene->make_node_locator();
	auto flags = int{};
	flags |= (ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow);
	if (node.entity_id && m_inspector.target == node.entity_id) { flags |= ImGuiTreeNodeFlags_Selected; }
	if (node.children().empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }
	auto tn = imcpp::TreeNode{node.name.c_str(), flags};
	if (node.entity_id) {
		auto target = Inspector::Target{.payload = *node.entity_id};
		if (ImGui::IsItemClicked()) { m_inspector.target = target; }
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			m_right_clicked = true;
			m_right_clicked_target = target;
		}
	}
	if (tn) {
		for (auto const& id : node.children()) {
			if (!walk_node(node_locator.get(id))) { return false; }
		}
	}
	return true;
}

void SceneGraph::draw_scene_tree(imcpp::OpenWindow) {
	auto node_locator = m_scene->make_node_locator();
	for (auto const& node : node_locator.roots()) {
		if (!walk_node(node_locator.get(node))) { return; }
	}

	if (m_right_clicked) {
		Popup::open("scene_graph.right_click");
		m_right_clicked = {};
	}
}

void SceneGraph::handle_popups() {
	if (auto popup = Popup{"scene_graph.spawn_entity"}) {
		ImGui::Text("Spawn Entity");
		m_entity_name("Name");
		if (!m_entity_name.empty() && ImGui::Button("Spawn")) {
			m_scene->spawn(std::string{m_entity_name.view()});
			m_entity_name = {};
			Popup::close_current();
		}
	}

	if (auto popup = Popup{"scene_graph.right_click"}) {
		auto const* entity_id = std::get_if<EntityId>(&m_right_clicked_target.payload);
		if (entity_id != nullptr && !m_scene->has_entity(*entity_id)) {
			m_right_clicked_target = {};
			return Popup::close_current();
		}
		if (ImGui::Selectable("Inspect")) {
			m_inspector.target = m_right_clicked_target;
			m_right_clicked_target = {};
			Popup::close_current();
		}
		if (entity_id != nullptr && ImGui::Selectable("Destroy")) {
			m_scene->get_entity(*entity_id).set_destroyed();
			m_right_clicked_target = {};
			Popup::close_current();
		}
	}
}
} // namespace spaced::imcpp
