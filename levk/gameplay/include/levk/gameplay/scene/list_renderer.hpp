#pragma once
#include <levk/engine/render/descriptor_helper.hpp>
#include <levk/engine/render/draw_list.hpp>
#include <levk/engine/render/pipeline.hpp>
#include <levk/graphics/render/pipeline_factory.hpp>
#include <levk/graphics/render/renderer.hpp>

#include <levk/engine/render/render_list.hpp>

namespace dens {
class registry;
}

namespace le {
class ListRenderer {
  public:
	using PipelineFactory = graphics::PipelineFactory;
	using Pipeline = graphics::Pipeline;
	using RenderPass = graphics::RenderPass;
	using DrawableMap = std::unordered_map<RenderPipeline, std::vector<Drawable>, RenderPipeline::Hasher>;

	static constexpr vk::Rect2D cast(DrawScissor rect) noexcept { return {{rect.offset.x, rect.offset.y}, {rect.extent.x, rect.extent.y}}; }
	static graphics::PipelineSpec pipelineSpec(RenderPipeline const& rp);
	static void add(DrawableMap& out_map, RenderPipeline const& rp, glm::mat4 const& model, MeshView const& mesh, std::optional<DrawScissor> scissor = {});

	void render(RenderPass& out_rp, AssetStore const& store, DrawableMap map);

  protected:
	virtual void fill(DrawableMap& out_map, AssetStore const& store, dens::registry const& registry);
	virtual void draw(DescriptorBinder bind, DrawList const& list, graphics::CommandBuffer const& cb) const;

	virtual void writeSets(DescriptorMap map, DrawList const& list) = 0;
};

class ListRenderer2 {
  public:
	using PipelineFactory = graphics::PipelineFactory;
	using Pipeline = graphics::Pipeline;
	using RenderPass = graphics::RenderPass;
	using RenderMap = std::unordered_map<RenderPipeline, graphics::DrawList, RenderPipeline::Hasher>;
	using Primitive = graphics::DrawPrimitive;

	static constexpr std::optional<vk::Rect2D> cast(std::optional<DrawScissor> rect) noexcept {
		return rect ? std::optional<vk::Rect2D>({{rect->offset.x, rect->offset.y}, {rect->extent.x, rect->extent.y}}) : std::nullopt;
	}
	static graphics::PipelineSpec pipelineSpec(RenderPipeline const& rp);
	static void add(RenderMap& out_map, RenderPipeline const& rp, glm::mat4 const& mat, Span<Primitive const> prims, std::optional<DrawScissor> scissor = {});

	void render(RenderPass& out_rp, AssetStore const& store, RenderMap map);

  protected:
	virtual void fill(RenderMap& out_map, AssetStore const& store, dens::registry const& registry);

	virtual void writeSets(DescriptorMap map, graphics::DrawList const& list) = 0;
	virtual void draw(DescriptorBinder bind, graphics::DrawList const& list, graphics::CommandBuffer const& cb) const = 0;
};
} // namespace le
