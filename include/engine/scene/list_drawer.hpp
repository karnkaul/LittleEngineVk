#pragma once
#include <unordered_set>
#include <core/utils/vbase.hpp>
#include <engine/scene/draw_list_factory.hpp>
#include <graphics/render/command_buffer.hpp>

namespace le {
class ListDrawer : public utils::VBase {
  public:
	static constexpr vk::Rect2D cast(Rect2D rect) noexcept { return {{rect.offset.x, rect.offset.y}, {rect.extent.x, rect.extent.y}}; }

	static void attach(decf::registry_t& registry, decf::entity_t entity, DrawLayer layer, Span<Primitive const> primitives);

	template <typename... Gen>
	void populate(decf::registry_t const& registry, bool sort = true);
	void draw(graphics::CommandBuffer cb) const;

	std::vector<DrawList> m_lists;

  private:
	virtual void draw(DrawList const&, graphics::CommandBuffer) const = 0;
};

// impl

template <typename... Gen>
void ListDrawer::populate(decf::registry_t const& registry, bool sort) {
	m_lists = DrawListFactory::template lists<Gen...>(registry, sort);
}
} // namespace le
