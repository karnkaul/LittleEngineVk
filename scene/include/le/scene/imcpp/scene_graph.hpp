#pragma once
#include <le/imcpp/input_text.hpp>
#include <le/scene/imcpp/scene_inspector.hpp>

namespace le::imcpp {
class SceneGraph {
  public:
	auto draw_to(NotClosed<Window> w, Scene& scene) -> SceneInspector::Target;

	[[nodiscard]] auto get_scene_inspector() const -> SceneInspector const& { return m_scene_inspector; }
	[[nodiscard]] auto get_scene_inspector() -> SceneInspector& { return m_scene_inspector; }

  private:
	auto check_stale() -> bool;
	void standalone_node(char const* label, SceneInspector::Type type);
	auto walk_node(Node& node) -> bool;
	void draw_scene_tree(NotClosed<Window> w);
	void handle_popups();

	SceneInspector m_scene_inspector{};
	Ptr<Scene> m_scene{};
	void const* m_prev{};

	bool m_right_clicked{};
	SceneInspector::Target m_right_clicked_target{};
	InputText<> m_entity_name{};
};
} // namespace le::imcpp
