#pragma once
#include <levk/engine/render/pipeline.hpp>
#include <levk/engine/render/render_list.hpp>
#include <levk/graphics/render/descriptor_helper.hpp>
#include <levk/graphics/render/pipeline_factory.hpp>
#include <levk/graphics/render/renderer.hpp>
#include <unordered_set>

namespace dens {
class registry;
}

namespace le {
class AssetStore;

class ListRenderer {
  public:
	struct PipeHasher {
		std::size_t operator()(graphics::Pipeline const& pipeline) const;
	};

	using PipelineFactory = graphics::PipelineFactory;
	using Pipeline = graphics::Pipeline;
	using RenderPass = graphics::RenderPass;
	using RenderMap = ktl::hash_table<RenderPipeline, graphics::DrawList, RenderPipeline::Hasher>;
	using Primitive = graphics::DrawPrimitive;
	using MatTexType = graphics::MatTexType;
	using PipeSet = std::unordered_set<Pipeline, PipeHasher>;

	static graphics::PipelineSpec pipelineSpec(RenderPipeline const& rp);

	[[nodiscard]] PipeSet render(RenderPass& out_rp, AssetStore const& store, RenderMap map);

  protected:
	virtual void fill(RenderMap& out_map, AssetStore const& store, dens::registry const& registry) const;
	virtual void draw(graphics::DescriptorHelper helper, graphics::DrawList const& list, graphics::CommandBuffer const& cb) = 0;
	void rotate(PipeSet const& pipes) const;

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
