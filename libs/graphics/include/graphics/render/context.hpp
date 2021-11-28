#pragma once
#include <core/colour.hpp>
#include <core/hash.hpp>
#include <graphics/command_buffer.hpp>
#include <graphics/common.hpp>
#include <graphics/draw_view.hpp>
#include <graphics/geometry.hpp>
#include <graphics/render/pipeline_factory.hpp>
#include <graphics/render/pipeline_flags.hpp>
#include <graphics/render/renderer.hpp>
#include <graphics/render/rgba.hpp>
#include <graphics/render/surface.hpp>
#include <graphics/screen_rect.hpp>
#include <graphics/utils/command_rotator.hpp>
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
	using GetSpirV = PipelineFactory::GetSpirV;

	static VertexInputInfo vertexInput(VertexInputCreateInfo const& info);
	static VertexInputInfo vertexInput(QuickVertexInput const& info);

	RenderContext(not_null<VRAM*> vram, GetSpirV&& gs, std::optional<VSync> vsync, Extent2D fbSize, Buffering bf = 2_B);

	std::unique_ptr<Renderer> defaultRenderer();
	void setRenderer(std::unique_ptr<Renderer>&& renderer) noexcept;

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
	PipelineFactory& pipelineFactory() noexcept { return m_pipelineFactory; }
	PipelineFactory const& pipelineFactory() const noexcept { return m_pipelineFactory; }
	VRAM& vram() noexcept { return *m_vram; }
	CommandRotator const& commands() const noexcept { return m_commandRotator; }
	RenderTarget const& previousFrame() const noexcept { return m_previousFrame; }
	std::optional<Image> previousFrameAsImage() const;

	struct Sync;

  private:
	bool submit(Span<vk::CommandBuffer const> cbs, Acquire const& acquired, Extent2D fbSize);

	Surface m_surface;
	PipelineFactory m_pipelineFactory;
	CommandRotator m_commandRotator;
	RingBuffer<Sync> m_syncs;
	RenderTarget m_previousFrame;
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
