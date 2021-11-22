#pragma once
#include <core/colour.hpp>
#include <core/hash.hpp>
#include <graphics/common.hpp>
#include <graphics/draw_view.hpp>
#include <graphics/geometry.hpp>
#include <graphics/render/command_buffer.hpp>
#include <graphics/render/pipeline.hpp>
#include <graphics/render/pipeline_flags.hpp>
#include <graphics/render/renderer.hpp>
#include <graphics/render/rgba.hpp>
#include <graphics/render/surface.hpp>
#include <graphics/screen_rect.hpp>
#include <memory>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace le::graphics {
struct QuickVertexInput;
struct VertexInputCreateInfo;

class RenderContext : public NoCopy {
  public:
	using Acquire = Surface::Acquire;
	using Attachment = Renderer::Attachment;

	static VertexInputInfo vertexInput(VertexInputCreateInfo const& info);
	static VertexInputInfo vertexInput(QuickVertexInput const& info);
	template <typename V = Vertex>
	static Pipeline::CreateInfo pipeInfo(PFlags flags = PFlags(PFlag::eDepthTest) | PFlag::eDepthWrite, f32 wire = 0.0f);

	RenderContext(not_null<VRAM*> vram, std::optional<VSync> vsync, Extent2D fbSize, Buffering buffering = 2_B);

	std::unique_ptr<Renderer> defaultRenderer();
	void setRenderer(std::unique_ptr<Renderer>&& renderer) noexcept { m_renderer = std::move(renderer); }

	Pipeline makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info);

	void waitForFrame();
	bool render(IDrawer& out_drawer, RenderBegin const& rb, Extent2D fbSize);
	bool recreateSwapchain(Extent2D fbSize, std::optional<VSync> vsync);

	Surface const& surface() const noexcept { return m_surface; }
	Buffering buffering() const noexcept { return m_buffering; }
	vk::Format colourImageFormat() const noexcept;
	ColourCorrection colourCorrection() const noexcept;
	f32 aspectRatio() const noexcept;
	bool supported(VSync vsync) const noexcept { return m_surface.vsyncs().test(vsync); }

	Renderer& renderer() const noexcept { return *m_renderer; }

	struct Sync;

  private:
	bool submit(Span<vk::CommandBuffer const> cbs, Acquire const& acquired, Extent2D fbSize);

	Surface m_surface;
	RingBuffer<Sync> m_syncs;
	Deferred<vk::PipelineCache> m_pipelineCache;
	not_null<VRAM*> m_vram;
	std::unique_ptr<Renderer> m_renderer;
	Buffering m_buffering;
};

struct RenderContext::Sync {
	static Sync make(not_null<Device*> device) {
		Sync ret;
		ret.draw = makeDeferred<vk::Semaphore>(device);
		ret.present = makeDeferred<vk::Semaphore>(device);
		ret.drawn = makeDeferred<vk::Fence>(device, true);
		return ret;
	}

	Deferred<vk::Semaphore> draw;
	Deferred<vk::Semaphore> present;
	Deferred<vk::Fence> drawn;
};

struct VertexInputCreateInfo {
	struct Member {
		vk::Format format = vk::Format::eR32G32B32Sfloat;
		std::size_t offset = 0;
	};
	struct Type {
		std::vector<Member> members;
		std::size_t size = 0;
		vk::VertexInputRate inputRate = vk::VertexInputRate::eVertex;
	};
	std::vector<Type> types;
	std::size_t bindStart = 0;
	std::size_t locationStart = 0;
};

struct QuickVertexInput {
	struct Attribute {
		vk::Format format = vk::Format::eR32G32B32Sfloat;
		std::size_t offset = 0;
	};

	u32 binding = 0;
	std::size_t size = 0;
	std::vector<Attribute> attributes;
};

// impl

template <typename V>
Pipeline::CreateInfo RenderContext::pipeInfo(PFlags flags, f32 wire) {
	Pipeline::CreateInfo ret;
	ret.fixedState.vertexInput = VertexInfoFactory<V>()(0);
	if (flags.test(PFlag::eDepthTest)) {
		ret.fixedState.depthStencilState.depthTestEnable = true;
		ret.fixedState.depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	}
	if (flags.test(PFlag::eDepthWrite)) { ret.fixedState.depthStencilState.depthWriteEnable = true; }
	if (flags.test(PFlag::eAlphaBlend)) {
		using CCF = vk::ColorComponentFlagBits;
		ret.fixedState.colorBlendAttachment.colorWriteMask = CCF::eR | CCF::eG | CCF::eB | CCF::eA;
		ret.fixedState.colorBlendAttachment.blendEnable = true;
		ret.fixedState.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		ret.fixedState.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		ret.fixedState.colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
		ret.fixedState.colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		ret.fixedState.colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		ret.fixedState.colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
	}
	if (wire > 0.0f) {
		ret.fixedState.rasterizerState.polygonMode = vk::PolygonMode::eLine;
		ret.fixedState.rasterizerState.lineWidth = wire;
	}
	return ret;
}

inline f32 RenderContext::aspectRatio() const noexcept {
	glm::ivec2 const ext = surface().extent();
	return f32(ext.x) / std::max(f32(ext.y), 1.0f);
}
inline ColourCorrection RenderContext::colourCorrection() const noexcept {
	return Surface::srgb(surface().format().colour.format) ? ColourCorrection::eAuto : ColourCorrection::eNone;
}
inline vk::Format RenderContext::colourImageFormat() const noexcept {
	return colourCorrection() == ColourCorrection::eAuto ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Snorm;
}
} // namespace le::graphics
