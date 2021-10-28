#include <core/maths.hpp>
#include <core/services.hpp>
#include <core/utils/enumerate.hpp>
#include <core/utils/string.hpp>
#include <engine/editor/controls/scene_tree.hpp>
#include <engine/editor/editor.hpp>
#include <engine/engine.hpp>
#include <engine/gui/widgets/dropdown.hpp>
#include <engine/scene/scene_registry.hpp>

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
TreeNode makeNode(std::string_view id, bool selected, bool leaf) { return TreeNode(id, selected, leaf, true, false); }

template <typename T>
bool operator==(ktl::either<decf::entity, gui::TreeRoot*>& either, T t) noexcept {
	if constexpr (std::is_convertible_v<T, decf::entity>) {
		return either.contains<decf::entity>() && either.get<decf::entity>() == t;
	} else {
		return either.contains<gui::TreeRoot*>() && either.get<gui::TreeRoot*>() == t;
	}
}

struct InspectVerifier {
	ktl::either<decf::entity, gui::TreeRoot*>& out;
	bool present{};

	InspectVerifier(ktl::either<decf::entity, gui::TreeRoot*>& out) noexcept : out(out) {}
	~InspectVerifier() noexcept {
		if (!present && (out.contains<gui::TreeRoot*>() || out.get<decf::entity>() != decf::entity())) { out = {}; }
	}

	template <typename T>
	bool operator()(T t) noexcept {
		if (out == t) {
			present = true;
			return true;
		}
		return false;
	}
};

template <typename T>
void inspect(InspectVerifier& iv, edi::TreeNode& tn, T t) {
	if (tn.test(GUI::eLeftClicked)) {
		if constexpr (std::is_convertible_v<T, decf::entity>) {
			iv.out = static_cast<decf::entity>(t);
		} else {
			iv.out = static_cast<gui::TreeRoot*>(t);
		}
		iv.present = true;
	} else if (iv.out == t && tn.test(GUI::eRightClicked)) {
		iv.out = {};
	}
}

template <typename T>
CStr<128> uniqueGuiName(T const& t) {
	CStr<128> str;
	if constexpr (std::is_base_of_v<gui::View, T>) {
		str = t.m_name;
	} else {
		str = utils::tName(&t);
	}
	str += CStr<128>("##");
	str += CStr<128>("%x", &t);
	return str;
}

void walk(SceneNode& node, InspectVerifier& iv, decf::registry& reg, Editor& editor) {
	if (reg.contains(node.entity())) {
		auto tn = makeNode(reg.name(node.entity()), iv(node.entity()), node.children().empty());
		if (tn.test(GUI::eOpen)) {
			for (not_null<SceneNode*> child : node.children()) { walk(*child, iv, reg, editor); }
		}
		inspect(iv, tn, node.entity());
	}
}

void walk(gui::TreeNode& node, InspectVerifier& iv, Editor& editor) {
	auto tn = makeNode(uniqueGuiName(node), iv(&node), node.nodes().empty());
	if (tn.test(GUI::eOpen)) {
		for (auto& node : node.nodes()) { walk(*node, iv, editor); }
	}
	inspect(iv, tn, &node);
}

void walk(gui::View& view, InspectVerifier& iv, Editor& editor) {
	auto tn = makeNode(uniqueGuiName(view), iv(&view), view.nodes().empty());
	if (tn.test(GUI::eOpen)) {
		for (auto& node : view.nodes()) { walk(*node, iv, editor); }
	}
	inspect(iv, tn, &view);
}

void walk(decf::spawn_t<gui::ViewStack> stack, InspectVerifier& iv, decf::registry const& reg, Editor& editor) {
	auto& st = stack.get<gui::ViewStack>();
	auto tn = makeNode(reg.name(stack), iv(stack), st.views().empty());
	if (tn.test(GUI::eOpen)) {
		for (auto it = st.views().rbegin(); it != st.views().rend(); ++it) { walk(**it, iv, editor); }
	}
	inspect(iv, tn, stack);
}
} // namespace
#endif

void SceneTree::update() {
#if defined(LEVK_USE_IMGUI)
	auto& editor = Services::get<Engine>()->editor();
	if (editor.m_in.registry) {
		auto& reg = *editor.m_in.registry;
		InspectVerifier iv(editor.m_out.inspecting);
		for (auto node : reg.root().children()) { walk(*node, iv, reg.registry(), editor); }
		for (auto query : reg.registry().view<gui::ViewStack>()) { walk(query, iv, reg.registry(), editor); }
		if (!editor.m_in.customEntities.empty()) {
			auto const tn = makeNode("[Custom]", false, false);
			if (tn.test(GUI::eOpen)) {
				for (auto const& entity : editor.m_in.customEntities) {
					if (entity != decf::entity() && reg.registry().contains(entity)) {
						auto tn = makeNode(reg.registry().name(entity), iv(entity), true);
						inspect(iv, tn, entity);
					}
				}
			}
		}
	}
#endif
}
} // namespace le::edi
