#include <levk/core/services.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/input/space.hpp>
#include <levk/engine/render/viewport.hpp>
#include <levk/gameplay/gui/tree.hpp>
#include <levk/graphics/font/font.hpp>
#include <levk/graphics/render/draw_list.hpp>
#include <levk/graphics/utils/utils.hpp>
#include <algorithm>

namespace le::gui {
DrawScissor scissor(input::Space const& space, glm::vec2 centre, glm::vec2 halfSize, bool normalised) noexcept {
	return {space.project(centre - halfSize, normalised) * space.render.scale + 0.5f, space.project(centre + halfSize, normalised) * space.render.scale + 0.5f};
}

not_null<graphics::VRAM*> TreeRoot::vram() const { return Services::get<graphics::VRAM>(); }

graphics::Font* TreeRoot::findFont(Hash uri) const {
	if (auto store = Services::find<AssetStore>()) {
		if (auto font = store->find<Font>(uri)) { return &*font; }
	}
	return {};
}

void TreeNode::update(input::Space const& space) {
	m_rect.adjust(m_parent->m_rect);
	m_scissor = scissor(space);
	onUpdate(space);
	for (auto& node : m_ts) { node->update(space); }
}

void TreeNode::pushDrawPrimitives(DrawList& out_list, Span<DrawPrimitive const> primitives) const {
	out_list.push(primitives, matrix(), graphics::utils::scissor(m_scissor));
}
} // namespace le::gui
