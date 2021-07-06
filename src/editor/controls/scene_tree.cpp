#include <engine/editor/controls/scene_tree.hpp>
#include <engine/editor/editor.hpp>
#include <engine/scene/scene_registry.hpp>

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
TreeNode makeNode(std::string_view id, bool selected, bool leaf) { return TreeNode(id, selected, leaf, true, false); }

void walk(SceneNode& node, decf::registry_t& reg) {
	auto& ins = Editor::s_out.inspecting;
	if (reg.contains(node.entity())) {
		auto tn = makeNode(reg.name(node.entity()), &node == ins.node, node.children().empty());
		if (tn.test(GUI::eOpen)) {
			for (not_null<SceneNode*> child : node.children()) { walk(*child, reg); }
		}
		if (tn.test(GUI::eLeftClicked)) {
			ins = {&node, node.entity()};
		} else if (ins.node == &node && tn.test(GUI::eRightClicked)) {
			ins = {};
		}
	}
}
} // namespace
#endif

void SceneTree::update() {
#if defined(LEVK_USE_IMGUI)
	if (Editor::s_in.registry) {
		auto& reg = *Editor::s_in.registry;
		for (auto node : reg.root().children()) { walk(*node, reg.registry()); }
		if (!Editor::s_in.customEntities.empty()) {
			auto const tn = makeNode("[Custom]", false, false);
			if (tn.test(GUI::eOpen)) {
				auto& ins = Editor::s_out.inspecting;
				for (auto const& entity : Editor::s_in.customEntities) {
					if (entity != decf::entity_t() && reg.registry().contains(entity)) {
						auto tn = makeNode(reg.registry().name(entity), entity == ins.entity, true);
						if (tn.test(GUI::eLeftClicked)) {
							ins = {nullptr, entity};
						} else if (ins.entity == entity && tn.test(GUI::eRightClicked)) {
							ins = {};
						}
					}
				}
			}
		}
	}
#endif
}
} // namespace le::edi
