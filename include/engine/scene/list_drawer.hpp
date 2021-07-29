#pragma once
#include <unordered_set>
#include <core/utils/vbase.hpp>
#include <engine/scene/draw_list_factory.hpp>
#include <graphics/render/command_buffer.hpp>

namespace le {
class ListDrawer : public utils::VBase {
  public:
	struct List {
		Span<Drawable const> drawables;
		not_null<graphics::Pipeline*> pipeline;
		Hash variant;
	};

	static constexpr vk::Rect2D cast(Rect2D rect) noexcept { return {{rect.offset.x, rect.offset.y}, {rect.extent.x, rect.extent.y}}; }

	static void attach(decf::registry& registry, decf::entity entity, DrawLayer layer, Span<Primitive const> primitives);

	template <typename... Gen>
	Span<List> populate(decf::registry const& registry, bool sort = true);
	void draw(graphics::CommandBuffer cb);

  private:
	virtual void draw(List const&, graphics::CommandBuffer) const = 0;

	std::vector<DrawList> m_drawLists;
	std::vector<List> m_lists;
};

// impl

template <typename... Gen>
Span<ListDrawer::List> ListDrawer::populate(decf::registry const& registry, bool sort) {
	m_drawLists = DrawListFactory::template lists<Gen...>(registry, sort);
	m_lists.clear();
	m_lists.reserve(m_drawLists.size());
	for (auto& list : m_drawLists) { m_lists.push_back({list.drawables, list.layer.pipeline, list.variant}); }
	return m_lists;
}
} // namespace le
