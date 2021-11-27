#pragma once
#include <core/utils/vbase.hpp>
#include <engine/render/descriptor_helper.hpp>
#include <engine/render/draw_list_factory.hpp>
#include <graphics/render/pipeline_factory.hpp>
#include <graphics/render/renderer.hpp>
#include <unordered_set>

namespace le {
class ListDrawer : public graphics::IDrawer {
  public:
	using PipelineFactory = graphics::PipelineFactory;
	using Pipeline = graphics::Pipeline;

	struct List {
		Span<Drawable const> drawables;
		Pipeline pipeline;
	};

	static constexpr vk::Rect2D cast(Rect2D rect) noexcept { return {{rect.offset.x, rect.offset.y}, {rect.extent.x, rect.extent.y}}; }

	template <typename... Gen>
	Span<List> populate(dens::registry const& registry, graphics::PipelineFactory& pf, vk::RenderPass rp, bool sort = true);
	void beginPass(PipelineFactory& pf, vk::RenderPass rp) override;
	void draw(graphics::CommandBuffer cb) override;

  protected:
	virtual void buildDrawLists(PipelineFactory& pf, vk::RenderPass rp) = 0;
	virtual void writeSets(DescriptorMap map, List const& list) = 0;
	virtual void draw(DescriptorBinder bind, List const& list, graphics::CommandBuffer cb) const = 0;

  private:
	void buildLists(graphics::PipelineFactory& pf, vk::RenderPass rp);

	std::vector<DrawList> m_drawLists;
	std::vector<List> m_lists;
};

// impl

template <typename... Gen>
Span<ListDrawer::List> ListDrawer::populate(dens::registry const& registry, graphics::PipelineFactory& pf, vk::RenderPass rp, bool sort) {
	m_drawLists = DrawListFactory::template lists<Gen...>(registry, sort);
	buildLists(pf, rp);
	return m_lists;
}
} // namespace le
