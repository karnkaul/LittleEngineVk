#include <core/services.hpp>
#include <core/utils/algo.hpp>
#include <editor/sudo.hpp>
#include <engine/cameras/freecam.hpp>
#include <engine/editor/inspector.hpp>
#include <engine/engine.hpp>
#include <engine/gui/view.hpp>
#include <engine/gui/widget.hpp>
#include <engine/render/draw_layer.hpp>
#include <engine/render/drawable.hpp>

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
struct TransformWidget {
	void operator()(Transform& transform) const { TWidget<Transform> tr("Pos", "Orn", "Scl", transform); }
};

struct GuiRect {
	void operator()(gui::Rect& rect) const {
		auto t = Text("Rect");
		TWidget<glm::vec2> org("Origin", rect.origin, false);
		TWidget<glm::vec2> size("Size", rect.size, false);
		t = Text("Anchor");
		TWidget<glm::vec2> ann("Norm", rect.anchor.norm, false, 0.001f);
		TWidget<glm::vec2> ano("Offset", rect.anchor.offset, false);
	}
};

struct GuiViewWidget {
	void operator()(gui::View& view) const {
		Text t("View");
		bool block = view.m_block == gui::View::Block::eBlock;
		TWidget<bool> blk("Block", block);
		view.m_block = block ? gui::View::Block::eBlock : gui::View::Block::eNone;
	}

	void operator()(gui::Widget& widget) const {
		Text t("Widget");
		TWidget<bool> blk("Interact", widget.m_interact);
	}
};

struct GuiNode {
	void operator()(gui::TreeNode& node) const {
		Text t("Node");
		TWidget<bool> active("Active", node.m_active);
		TWidget<f32> zi("Z-Index", node.m_zIndex);
		TWidget<glm::quat> orn("Orient", node.m_orientation);
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

void Inspector::update([[maybe_unused]] SceneRef const& scene) {
#if defined(LEVK_USE_IMGUI)
	if (scene.valid()) {
		auto inspect = Sudo::inspect(scene);
		if (inspect->contains<dens::entity>()) {
			if (auto entity = inspect->get<dens::entity>(); entity != dens::entity()) {
				auto& reg = *Sudo::registry(scene);
				Text(reg.name(entity));
				if (reg.attached<DrawLayer>(entity)) {
					TWidgetWrap<bool> draw;
					if (draw(shouldDraw(entity, reg), "Draw", draw.out)) { shouldDraw(entity, reg, draw.out); }
				}
				Styler s{Style::eSeparator};
				if (auto transform = reg.find<Transform>(entity)) {
					TransformWidget{}(*transform);
					s();
				}
				for (auto& gadget : s_gadgets) {
					if ((*gadget)(entity, reg)) { s(); }
				}
			}
		} else {
			auto tr = Sudo::inspect(scene)->get<gui::TreeRoot*>();
			Text txt("GUI");
			GuiRect{}(tr->m_rect);
			if (auto view = dynamic_cast<gui::View*>(tr)) {
				GuiViewWidget{}(*view);
			} else if (auto node = dynamic_cast<gui::TreeNode*>(tr)) {
				GuiNode{}(*node);
				if (auto widget = dynamic_cast<gui::Widget*>(tr)) { GuiViewWidget{}(*widget); }
			}
			Styler s{Style::eSeparator};
			for (auto& gadget : s_guiGadgets) {
				if ((*gadget)(*tr)) { s(); }
			}
		}
	}
#endif
}

void Inspector::clear() {
	s_gadgets.clear();
	s_guiGadgets.clear();
}
} // namespace le::edi
