#pragma once
#include <le/imcpp/input_text.hpp>
#include <le/scene/imcpp/inspector.hpp>

namespace le::imcpp {
class SceneGraph {
  public:
	auto draw_to(NotClosed<Window> w, Scene& scene) -> Inspector::Target;

	[[nodiscard]] auto inspector() const -> Inspector const& { return m_inspector; }
	auto inspector() -> Inspector& { return m_inspector; }

  private:
	auto check_stale() -> bool;
	void standalone_node(char const* label, Inspector::Type type);
	auto walk_node(Node& node) -> bool;
	void draw_scene_tree(NotClosed<Window> w);
	void handle_popups();

	Inspector m_inspector{};
	Ptr<Scene> m_scene{};
	void const* m_prev{};

	bool m_right_clicked{};
	Inspector::Target m_right_clicked_target{};
	InputText<> m_entity_name{};
};
} // namespace le::imcpp
