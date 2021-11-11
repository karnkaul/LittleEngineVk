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
bool operator==(Inspecting& either, T t) noexcept {
	if constexpr (std::is_convertible_v<T, dens::entity>) {
		return either.contains<dens::entity>() && either.get<dens::entity>() == t;
	} else {
		return either.contains<gui::TreeRoot*>() && either.get<gui::TreeRoot*>() == t;
	}
}

struct InspectVerifier {
	Inspecting& out;
	bool present{};

	InspectVerifier(Inspecting& out) noexcept : out(out) {}
	~InspectVerifier() noexcept {
		if (!present && (out.contains<gui::TreeRoot*>() || out.get<dens::entity>() != dens::entity())) { out = {}; }
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
		if constexpr (std::is_convertible_v<T, dens::entity>) {
			iv.out = static_cast<dens::entity>(t);
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

void walk(SceneNode& node, InspectVerifier& iv, dens::registry const& reg) {
	if (reg.contains(node.entity())) {
		auto tn = makeNode(reg.name(node.entity()), iv(node.entity()), node.nodes().empty());
		if (tn.test(GUI::eOpen)) {
			for (dens::entity child : node.nodes()) {
				if (auto n = reg.find<SceneNode>(child)) { walk(*n, iv, reg); }
			}
		}
		inspect(iv, tn, node.entity());
	}
}

void walk(gui::TreeNode& node, InspectVerifier& iv) {
	auto tn = makeNode(uniqueGuiName(node), iv(&node), node.nodes().empty());
	if (tn.test(GUI::eOpen)) {
		for (auto& node : node.nodes()) { walk(*node, iv); }
	}
	inspect(iv, tn, &node);
}

void walk(gui::View& view, InspectVerifier& iv) {
	auto tn = makeNode(uniqueGuiName(view), iv(&view), view.nodes().empty());
	if (tn.test(GUI::eOpen)) {
		for (auto& node : view.nodes()) { walk(*node, iv); }
	}
	inspect(iv, tn, &view);
}

void walk(dens::entity_view<gui::ViewStack> stack, InspectVerifier& iv, dens::registry const& reg) {
	auto& st = stack.get<gui::ViewStack>();
	auto tn = makeNode(reg.name(stack), iv(stack), st.views().empty());
	if (tn.test(GUI::eOpen)) {
		for (auto it = st.views().rbegin(); it != st.views().rend(); ++it) { walk(**it, iv); }
	}
	inspect(iv, tn, stack);
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
						inspect(iv, tn, entity);
					}
				}
			}
		}
	}
#endif
}

void SceneTree::clear() { s_custom.clear(); }
} // namespace le::edi
