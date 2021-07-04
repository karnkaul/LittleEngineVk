#pragma once
#include <memory>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <core/colour.hpp>
#include <core/hash.hpp>
#include <graphics/common.hpp>
#include <graphics/draw_view.hpp>
#include <graphics/geometry.hpp>
#include <graphics/render/command_pool.hpp>
#include <graphics/render/pipeline.hpp>
#include <graphics/render/renderer.hpp>
#include <graphics/render/rgba.hpp>
#include <graphics/screen_rect.hpp>

namespace le::graphics {
struct QuickVertexInput;
struct VertexInputCreateInfo;

enum class PFlag { eDepthTest, eDepthWrite, eAlphaBlend, eCOUNT_ };
using PFlags = kt::enum_flags<PFlag>;

class RenderContext : NoCopy {
  public:
	enum class Status { eIdle, eWaiting, eReady, eBegun, eEnded, eDrawing, eCOUNT_ };

	using Frame = ARenderer::Draw;

	static VertexInputInfo vertexInput(VertexInputCreateInfo const& info);
	static VertexInputInfo vertexInput(QuickVertexInput const& info);
	template <typename V = Vertex>
	static Pipeline::CreateInfo pipeInfo(PFlags flags = PFlags(PFlag::eDepthTest) | PFlag::eDepthWrite);

	RenderContext(not_null<Swapchain*> swapchain, std::unique_ptr<ARenderer>&& renderer);

	Pipeline makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info);

	bool ready(glm::ivec2 framebufferSize);
	bool waitForFrame();
	std::optional<Frame> beginFrame();
	bool beginDraw(graphics::FrameDrawer& out_drawer, ScreenView const& view, RGBA clear, vk::ClearDepthStencilValue depth = {1.0f, 0});
	bool endDraw();
	bool endFrame();
	bool submitFrame();

	Status status() const noexcept { return m_storage.status; }
	std::size_t index() const noexcept { return m_storage.renderer->index(); }
	Buffering buffering() const noexcept { return m_storage.renderer->buffering(); }
	Extent2D extent() const noexcept { return m_swapchain->display().extent; }
	vk::SurfaceFormatKHR swapchainFormat() const noexcept { return m_swapchain->colourFormat(); }
	vk::Format colourImageFormat() const noexcept;
	ColourCorrection colourCorrection() const noexcept;
	f32 aspectRatio() const noexcept;
	glm::mat4 preRotate() const noexcept;
	vk::Viewport viewport(Extent2D extent = {0, 0}, ScreenView const& view = {}, glm::vec2 depth = {0.0f, 1.0f}) const noexcept;
	vk::Rect2D scissor(Extent2D extent = {0, 0}, ScreenView const& view = {}) const noexcept;

	ARenderer& renderer() const noexcept { return *m_storage.renderer; }
	CommandPool const& commandPool() const noexcept { return m_pool; }

  private:
	template <typename... T>
	bool check(T... any) noexcept {
		return ((m_storage.status == any) || ...);
	}
	void set(Status s) noexcept { m_storage.status = s; }

	struct Storage {
		std::unique_ptr<ARenderer> renderer;
		std::optional<Frame> frame;
		Deferred<vk::PipelineCache> pipelineCache;
		Status status = {};
	};

	Storage m_storage;
	CommandPool m_pool;
	not_null<Swapchain*> m_swapchain;
	not_null<Device*> m_device;
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
Pipeline::CreateInfo RenderContext::pipeInfo(PFlags flags) {
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
	return ret;
}

inline f32 RenderContext::aspectRatio() const noexcept {
	glm::ivec2 const ext = extent();
	return f32(ext.x) / std::max(f32(ext.y), 1.0f);
}
inline ColourCorrection RenderContext::colourCorrection() const noexcept {
	return Swapchain::srgb(swapchainFormat().format) ? ColourCorrection::eAuto : ColourCorrection::eNone;
}
inline vk::Format RenderContext::colourImageFormat() const noexcept {
	return colourCorrection() == ColourCorrection::eAuto ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Snorm;
}
} // namespace le::graphics
