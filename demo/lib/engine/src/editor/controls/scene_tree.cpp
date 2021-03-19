#include <engine/editor/controls/scene_tree.hpp>
#include <engine/editor/editor.hpp>

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
void walk(SceneNode& node, decf::registry_t& reg) {
	auto& ins = Editor::s_out.inspecting;
	if (reg.contains(node.entity())) {
		auto tn = TreeNode(reg.name(node.entity()), &node == ins.node, node.children().empty(), true, false);
		if (tn.test(GUI::eOpen)) {
			for (SceneNode& child : node.children()) {
				walk(child, reg);
			}
		}
		if (tn.test(GUI::eLeftClicked)) {
			ins.node = &node;
			ins.entity = node.entity();
		} else if (ins.node == &node && tn.test(GUI::eRightClicked)) {
			ins = {};
		}
	}
}
} // namespace
#endif

void SceneTree::update() {
#if defined(LEVK_USE_IMGUI)
	if (Editor::s_in.root && Editor::s_in.registry) {
		for (SceneNode& node : Editor::s_in.root->children()) {
			walk(node, *Editor::s_in.registry);
		}
	}
#endif
}
} // namespace le::edi
