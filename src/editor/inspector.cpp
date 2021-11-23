#include <core/services.hpp>
#include <core/utils/algo.hpp>
#include <editor/sudo.hpp>
#include <engine/cameras/freecam.hpp>
#include <engine/editor/inspector.hpp>
#include <engine/engine.hpp>
#include <engine/gui/view.hpp>
#include <engine/gui/widget.hpp>
#include <engine/render/draw_group.hpp>
#include <engine/render/drawable.hpp>

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
struct TransformWidget {
	void operator()(Transform& transform) const {
		TWidget<Transform> tr("Pos", "Orn", "Scl", transform);
		Styler s{Style::eSeparator};
	}
};

struct GuiRect {
	void operator()(gui::Rect& rect) const {
		auto t = Text("Rect");
		TWidget<glm::vec2> org("Origin", rect.origin, false);
		TWidget<glm::vec2> size("Size", rect.size, false);
		t = Text("Anchor");
		TWidget<glm::vec2> ann("Norm", rect.anchor.norm, false, 0.001f);
		TWidget<glm::vec2> ano("Offset", rect.anchor.offset, false);
		Styler s{Style::eSeparator};
	}
};

struct GuiViewWidget {
	void operator()(gui::View& view) const {
		Text t("View");
		bool block = view.m_block == gui::View::Block::eBlock;
		TWidget<bool> blk("Block", block);
		view.m_block = block ? gui::View::Block::eBlock : gui::View::Block::eNone;
		Styler s{Style::eSeparator};
	}

	void operator()(gui::Widget& widget) const {
		Text t("Widget");
		TWidget<bool> blk("Interact", widget.m_interact);
		Styler s{Style::eSeparator};
	}
};

struct GuiNode {
	void operator()(gui::TreeNode& node) const {
		Text t("Node");
		TWidget<bool> active("Active", node.m_active);
		TWidget<f32> zi("Z-Index", node.m_zIndex);
		TWidget<glm::quat> orn("Orient", node.m_orientation);
		Styler s{Style::eSeparator};
	}
};

bool shouldDraw(dens::entity e, dens::registry const& r) { return !r.attached<NoDraw>(e); }

void shouldDraw(dens::entity e, dens::registry& r, bool set) {
	if (set) {
		r.detach<NoDraw>(e);
	} else {
		r.attach<NoDraw>(e);
	}
}
} // namespace
#endif

bool Inspector::detach(std::string const& id) {
	auto doDetach = [id](auto& map) {
		if (auto it = map.find(id); it != map.end()) {
			map.erase(it);
			return true;
		}
		return false;
	};
	return doDetach(s_gadgets) || doDetach(s_guiGadgets);
}

void Inspector::update([[maybe_unused]] SceneRef const& scene) {
#if defined(LEVK_USE_IMGUI)
	if (scene.valid()) {
		auto inspect = Sudo::inspect(scene);
		if (inspect->entity != dens::entity()) {
			auto& reg = *Sudo::registry(scene);
			if (!inspect->tree) {
				Text(reg.name(inspect->entity));
				Text(ktl::stack_string<16>("id: %u", inspect->entity.id));
				if (reg.attached<DrawGroup>(inspect->entity)) {
					TWidgetWrap<bool> draw;
					if (draw(shouldDraw(inspect->entity, reg), "Draw", draw.out)) { shouldDraw(inspect->entity, reg, draw.out); }
				}
				Styler s{Style::eSeparator};
				if (auto transform = reg.find<Transform>(inspect->entity)) { TransformWidget{}(*transform); }
				attach(inspect->entity, reg);
			} else {
				auto const name = ktl::stack_string<128>("%s -> [GUI node]", reg.name(inspect->entity).data());
				Text txt(name.data());
				GuiRect{}(inspect->tree->m_rect);
				if (auto view = dynamic_cast<gui::View*>(inspect->tree)) {
					GuiViewWidget{}(*view);
				} else if (auto node = dynamic_cast<gui::TreeNode*>(inspect->tree)) {
					GuiNode{}(*node);
					if (auto widget = dynamic_cast<gui::Widget*>(inspect->tree)) { GuiViewWidget{}(*widget); }
				}
				for (auto const& [id, gadget] : s_guiGadgets) { gadget->inspect(id, inspect->entity, reg, inspect->tree); }
			}
		}
	}
#endif
}

void Inspector::clear() {
	s_gadgets.clear();
	s_guiGadgets.clear();
}

void Inspector::attach(dens::entity entity, dens::registry& reg) {
	std::vector<GadgetMap::const_iterator> attachable;
	attachable.reserve(s_gadgets.size());
	for (auto it = s_gadgets.cbegin(); it != s_gadgets.cend(); ++it) {
		auto const& [id, gadget] = *it;
		if (!gadget->inspect(id, entity, reg, {}) && gadget->attachable()) { attachable.push_back(it); }
	}
	if (!attachable.empty()) {
		Styler(glm::vec2{0.0f, 30.0f});
		if (Button("Attach")) { Popup::open("attach_component"); }
		if (auto attach = Popup("attach_component")) {
			static ktl::stack_string<128> s_filter;
			edi::TWidget<char*>("Search##component_filter", s_filter.c_str(), s_filter.capacity());
			auto filter = s_filter.get();
			for (auto const& kvp : attachable) {
				auto const& [id, gadget] = *kvp;
				Pane sub("component_list", {0.0f, 80.0f});
				if ((filter.empty() || id.find(filter) != std::string_view::npos) && Selectable(id.data())) {
					gadget->attach(entity, reg);
					attach.close();
				}
			}
		}
	}
}
} // namespace le::edi
