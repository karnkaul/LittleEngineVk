#include <core/services.hpp>
#include <core/utils/algo.hpp>
#include <engine/cameras/freecam.hpp>
#include <engine/editor/controls/inspector.hpp>
#include <engine/engine.hpp>
#include <engine/scene/scene_registry.hpp>

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
struct Transform {
	bool operator()(SceneNode& node, decf::registry&) const {
		auto tr = TWidget<SceneNode>("Pos", "Orn", "Scl", node);
		return true;
	}
};
} // namespace
#endif

void Inspector::update() {
#if defined(LEVK_USE_IMGUI)
	auto& editor = Services::locate<Engine>()->editor();
	if (editor.m_in.registry && editor.m_out.inspecting.entity != decf::entity()) {
		auto entity = editor.m_out.inspecting.entity;
		auto node = editor.m_out.inspecting.node;
		auto& registry = editor.m_in.registry->registry();
		Text(registry.name(entity));
		TWidgetWrap<bool> enb;
		if (enb(registry.enabled(entity), "Enabled", enb.out)) { registry.enable(entity, enb.out); }
		Styler s{Style::eSeparator};
		static std::string const trID("Transform");
		if (node) {
			Transform tr;
			tr(*node, registry);
		}
		for (auto& [id, gadget] : s_gadgets) {
			if ((*gadget)(entity, registry)) { s(); }
		}
	}
#endif
}
} // namespace le::edi
