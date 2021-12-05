#pragma once
#include <engine/render/descriptor_helper.hpp>
#include <engine/render/draw_list.hpp>
#include <engine/render/layer.hpp>
#include <graphics/render/pipeline_factory.hpp>
#include <graphics/render/renderer.hpp>

namespace dens {
class registry;
}

namespace le {
class ListDrawer : public graphics::IDrawer {
  public:
	using PipelineFactory = graphics::PipelineFactory;
	using Pipeline = graphics::Pipeline;
	using LayerMap = std::unordered_map<RenderLayer, std::vector<Drawable>, RenderLayer::Hasher>;

	static constexpr vk::Rect2D cast(DrawScissor rect) noexcept { return {{rect.offset.x, rect.offset.y}, {rect.extent.x, rect.extent.y}}; }
	static void add(LayerMap& out_map, RenderLayer const& layer, glm::mat4 const& model, MeshView const& mesh, DrawScissor scissor = {});

	void beginPass(PipelineFactory& pf, vk::RenderPass rp) override final;
	void draw(graphics::CommandBuffer cb) override final;

  protected:
	virtual void fill(LayerMap& out_map, dens::registry const& registry);
	virtual void draw(DescriptorBinder bind, DrawList const& list, graphics::CommandBuffer cb) const;

	virtual void populate(LayerMap& out_map) = 0;
	virtual void writeSets(DescriptorMap map, DrawList const& list) = 0;

  private:
	std::vector<DrawList> m_drawLists;
};
} // namespace le
