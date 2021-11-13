#include <core/maths.hpp>
#include <core/services.hpp>
#include <core/utils/enumerate.hpp>
#include <core/utils/string.hpp>
#include <editor/sudo.hpp>
#include <engine/editor/editor.hpp>
#include <engine/editor/scene_tree.hpp>
#include <engine/engine.hpp>
#include <engine/gui/view.hpp>
#include <engine/gui/widgets/dropdown.hpp>

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
TreeNode makeNode(std::string_view id, bool selected, bool leaf) { return TreeNode(id, selected, leaf, true, false); }

template <typename T>
bool operator==(Inspecting& inspecting, T t) noexcept {
	if constexpr (std::is_convertible_v<T, dens::entity>) {
		return inspecting.entity == t;
	} else {
		return inspecting.tree == t;
	}
}

struct InspectVerifier {
	Inspecting& out;
	bool present{};

	InspectVerifier(Inspecting& out) noexcept : out(out) {}
	~InspectVerifier() noexcept {
		if (!present && (out.tree || out.entity != dens::entity())) { out = {}; }
	}

	bool operator()(gui::TreeRoot* node) noexcept {
		if (out.tree == node) {
			present = true;
			return true;
		}
		return false;
	}

	bool operator()(dens::entity entity) noexcept {
		if (entity == out.entity && !out.tree) {
			present = true;
			return true;
		}
		return false;
	}
};

void inspect(InspectVerifier& iv, edi::TreeNode& tn, dens::entity entity, gui::TreeRoot* node) {
	if (tn.test(GUI::eLeftClicked)) {
		iv.out.entity = entity;
		iv.out.tree = node;
		iv.present = true;
	} else if (iv.out.entity == entity && iv.out.tree == node && tn.test(GUI::eRightClicked)) {
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

void walk(SceneNode& node, InspectVerifier& iv, dens::registry const& reg) {
	if (reg.contains(node.entity())) {
		auto tn = makeNode(reg.name(node.entity()), iv(node.entity()), node.nodes().empty());
		if (tn.test(GUI::eOpen)) {
			for (dens::entity child : node.nodes()) {
				if (auto n = reg.find<SceneNode>(child)) { walk(*n, iv, reg); }
			}
		}
		inspect(iv, tn, node.entity(), {});
	}
}

void walk(gui::TreeNode& node, InspectVerifier& iv, dens::entity entity) {
	auto tn = makeNode(uniqueGuiName(node), iv(&node), node.nodes().empty());
	if (tn.test(GUI::eOpen)) {
		for (auto& node : node.nodes()) { walk(*node, iv, entity); }
	}
	inspect(iv, tn, entity, &node);
}

void walk(gui::View& view, InspectVerifier& iv, dens::entity entity) {
	auto tn = makeNode(uniqueGuiName(view), iv(&view), view.nodes().empty());
	if (tn.test(GUI::eOpen)) {
		for (auto& node : view.nodes()) { walk(*node, iv, entity); }
	}
	inspect(iv, tn, entity, &view);
}

void walk(dens::entity_view<gui::ViewStack> stack, InspectVerifier& iv, dens::registry const& reg) {
	auto& st = stack.get<gui::ViewStack>();
	auto tn = makeNode(reg.name(stack), iv(stack), st.views().empty());
	if (tn.test(GUI::eOpen)) {
		for (auto it = st.views().rbegin(); it != st.views().rend(); ++it) { walk(**it, iv, stack); }
	}
	inspect(iv, tn, stack, {});
}
} // namespace
#endif

void SceneTree::attach([[maybe_unused]] Span<dens::entity const> entities) {
#if defined(LEVK_USE_IMGUI)
	for (auto e : entities) { s_custom.insert(e); }
#endif
}

void SceneTree::detach([[maybe_unused]] Span<dens::entity const> entities) {
#if defined(LEVK_USE_IMGUI)
	for (auto e : entities) { s_custom.erase(e); }
#endif
}

void SceneTree::update([[maybe_unused]] SceneRef scene) {
#if defined(LEVK_USE_IMGUI)
	if (scene.valid()) {
		auto& reg = scene.registry();
		InspectVerifier iv(*Sudo::inspect(scene));
		if (auto root = reg.find<SceneNode>(scene.root())) {
			for (auto node : root->nodes()) {
				if (auto n = reg.find<SceneNode>(node)) { walk(*n, iv, reg); }
			}
		}
		for (auto query : reg.view<gui::ViewStack>()) { walk(query, iv, reg); }
		if (!s_custom.empty()) {
			auto const tn = makeNode("[Custom]", false, false);
			if (tn.test(GUI::eOpen)) {
				for (auto entity : s_custom) {
					if (entity != dens::entity() && reg.contains(entity)) {
						auto tn = makeNode(reg.name(entity), iv(entity), true);
						inspect(iv, tn, entity, {});
					}
				}
			}
		}
	}
#endif
}

void SceneTree::clear() { s_custom.clear(); }
} // namespace le::edi
