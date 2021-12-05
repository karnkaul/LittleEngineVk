#pragma once
#include <engine/render/descriptor_helper.hpp>
#include <engine/render/draw_group.hpp>
#include <engine/render/draw_list.hpp>
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
	using GroupMap = std::unordered_map<DrawGroup, std::vector<Drawable>, DrawGroup::Hasher>;

	static constexpr vk::Rect2D cast(DrawScissor rect) noexcept { return {{rect.offset.x, rect.offset.y}, {rect.extent.x, rect.extent.y}}; }
	static void add(GroupMap& out_map, DrawGroup const& group, glm::mat4 const& model, MeshView const& mesh, DrawScissor scissor = {});

	void beginPass(PipelineFactory& pf, vk::RenderPass rp) override final;
	void draw(graphics::CommandBuffer cb) override final;

  protected:
	virtual void fill(GroupMap& out_map, dens::registry const& registry);
	virtual void draw(DescriptorBinder bind, DrawList const& list, graphics::CommandBuffer cb) const;

	virtual void populate(GroupMap& out_map) = 0;
	virtual void writeSets(DescriptorMap map, DrawList const& list) = 0;

  private:
	std::vector<DrawList> m_drawLists;
};
} // namespace le
