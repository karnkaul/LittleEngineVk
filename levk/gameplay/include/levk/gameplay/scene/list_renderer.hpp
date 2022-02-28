#pragma once
#include <levk/engine/render/descriptor_helper.hpp>
#include <levk/engine/render/pipeline.hpp>
#include <levk/engine/render/render_list.hpp>
#include <levk/graphics/render/pipeline_factory.hpp>
#include <levk/graphics/render/renderer.hpp>

namespace dens {
class registry;
}

namespace le {
class ListRenderer {
  public:
	using PipelineFactory = graphics::PipelineFactory;
	using Pipeline = graphics::Pipeline;
	using RenderPass = graphics::RenderPass;
	using RenderMap = std::unordered_map<RenderPipeline, graphics::DrawList, RenderPipeline::Hasher>;
	using Primitive = graphics::DrawPrimitive;
	using MatTexType = graphics::MatTexType;

	static graphics::PipelineSpec pipelineSpec(RenderPipeline const& rp);

	void render(RenderPass& out_rp, AssetStore const& store, RenderMap map);

  protected:
	virtual void writeSets(DescriptorMap map, graphics::DrawList const& list) = 0;
	virtual void draw(DescriptorBinder bind, graphics::DrawList const& list, graphics::CommandBuffer const& cb) const = 0;

	virtual void fill(RenderMap& out_map, AssetStore const& store, dens::registry const& registry);

	vk::Rect2D m_scissor{};
};

struct DrawListGen {
	// Populates DrawGroup + [DynamicMesh, MeshProvider, gui::ViewStack]
	void operator()(ListRenderer::RenderMap& map, AssetStore const& store, dens::registry const& registry) const;
};

struct DebugDrawListGen {
	inline static bool populate_v = levk_debug;

	// Populates DrawGroup + [physics::Trigger::Debug]
	void operator()(ListRenderer::RenderMap& map, AssetStore const& store, dens::registry const& registry) const;
};
} // namespace le
