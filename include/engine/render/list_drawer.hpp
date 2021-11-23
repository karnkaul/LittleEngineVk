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

	struct List {
		Span<Drawable const> drawables;
		not_null<graphics::Pipeline*> pipeline;
		Hash variant;
	};

	static constexpr vk::Rect2D cast(Rect2D rect) noexcept { return {{rect.offset.x, rect.offset.y}, {rect.extent.x, rect.extent.y}}; }

	template <typename... Gen>
	Span<List> populate(dens::registry const& registry, bool sort = true);
	void beginPass(PipelineFactory& pf, vk::RenderPass rp) override;
	void draw(graphics::CommandBuffer cb) override;

  private:
	virtual void buildDrawLists(PipelineFactory& pf, vk::RenderPass rp) = 0;
	virtual void writeSets(DescriptorMap map, List const& list) = 0;
	virtual void draw(DescriptorBinder bind, List const& list, graphics::CommandBuffer cb) const = 0;

	std::vector<DrawList> m_drawLists;
	std::vector<List> m_lists;
};

class ListDrawer2 : public graphics::IDrawer {
  public:
	using PipelineFactory = graphics::PipelineFactory;
	using Pipeline = PipelineFactory::Data;

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

	std::vector<DrawList2> m_drawLists;
	std::vector<List> m_lists;
};

// impl

template <typename... Gen>
Span<ListDrawer::List> ListDrawer::populate(dens::registry const& registry, bool sort) {
	m_drawLists = DrawListFactory::template lists<Gen...>(registry, sort);
	m_lists.clear();
	m_lists.reserve(m_drawLists.size());
	for (auto& list : m_drawLists) { m_lists.push_back({list.drawables, list.layer.pipeline, list.variant}); }
	return m_lists;
}

template <typename... Gen>
Span<ListDrawer2::List> ListDrawer2::populate(dens::registry const& registry, graphics::PipelineFactory& pf, vk::RenderPass rp, bool sort) {
	m_drawLists = DrawListFactory::template lists2<Gen...>(registry, sort);
	buildLists(pf, rp);
	return m_lists;
}
} // namespace le
