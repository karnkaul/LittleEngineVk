#include <core/utils/algo.hpp>
#include <engine/editor/controls/inspector.hpp>
#include <engine/editor/editor.hpp>

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
struct Transform : Gadget {
	bool show(SceneNode const&, decf::registry_t const&) const override {
		return true;
	}

	void operator()(SceneNode& node, decf::registry_t&) override {
		auto tr = TWidget<SceneNode>("Pos", "Orn", "Scl", node);
	}
};
} // namespace
#endif

void Inspector::update() {
#if defined(LEVK_USE_IMGUI)
	if (Editor::s_out.inspecting.node && Editor::s_in.registry) {
		auto entity = Editor::s_out.inspecting.entity;
		auto& node = *Editor::s_out.inspecting.node;
		auto& registry = *Editor::s_in.registry;
		Text(registry.name(entity));
		TWidgetWrap<bool> enb;
		if (enb(registry.enabled(entity), "Enabled", enb.out)) {
			registry.enable(entity, enb.out);
		}
		Styler s{Style::eSeparator};
		static std::string const trID("Transform");
		if (!utils::contains(s_gadgets, trID)) {
			attach<Transform>(trID);
		}
		for (auto& [id, gadget] : s_gadgets) {
			if (gadget->show(node, registry)) {
				if (auto tn = TreeNode(id)) {
					(*gadget)(node, registry);
					s();
				}
			}
		}
	}
#endif
}
} // namespace le::edi
