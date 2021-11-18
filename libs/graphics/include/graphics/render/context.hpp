#pragma once
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
#include <memory>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace le::graphics {
struct QuickVertexInput;
struct VertexInputCreateInfo;

enum class PFlag { eDepthTest, eDepthWrite, eAlphaBlend };
using PFlags = ktl::enum_flags<PFlag, u8>;
constexpr PFlags pflags_all = PFlags(PFlag::eDepthTest, PFlag::eDepthWrite, PFlag::eAlphaBlend);
static_assert(pflags_all.count() == 3, "Invariant violated");

class RenderContext : NoCopy {
  public:
	enum class Status { eIdle, eWaiting, eReady, eBegun, eEnded, eDrawing };

	static VertexInputInfo vertexInput(VertexInputCreateInfo const& info);
	static VertexInputInfo vertexInput(QuickVertexInput const& info);
	template <typename V = Vertex>
	static Pipeline::CreateInfo pipeInfo(PFlags flags = PFlags(PFlag::eDepthTest) | PFlag::eDepthWrite, f32 wire = 0.0f);

	RenderContext(not_null<VRAM*> vram, Swapchain::CreateInfo const& info, glm::vec2 fbSize);

	template <typename T, typename... Args>
	void makeRenderer(Args&&... args);

	Pipeline makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info);

	bool ready(glm::ivec2 framebufferSize);
	bool waitForFrame();
	std::optional<RenderTarget> beginFrame();
	std::optional<CommandBuffer> beginDraw(ScreenView const& view, RGBA clear, ClearDepth depth = {1.0f, 0});
	bool endDraw();
	bool endFrame();
	bool submitFrame();

	void reconstruct(std::optional<graphics::Vsync> vsync = std::nullopt);

	Status status() const noexcept { return m_storage.status; }
	std::size_t index() const noexcept { return m_storage.renderer->index(); }
	Buffering buffering() const noexcept { return m_storage.renderer->buffering(); }
	Extent2D extent() const noexcept { return m_swapchain.display().extent; }
	vk::SurfaceFormatKHR swapchainFormat() const noexcept { return m_swapchain.colourFormat(); }
	Vsync vsync() const noexcept { return m_swapchain.vsync(); }
	vk::Format colourImageFormat() const noexcept;
	ColourCorrection colourCorrection() const noexcept;
	f32 aspectRatio() const noexcept;
	glm::mat4 preRotate() const noexcept;
	vk::Viewport viewport(Extent2D extent = {0, 0}, ScreenView const& view = {}, glm::vec2 depth = {0.0f, 1.0f}) const noexcept;
	vk::Rect2D scissor(Extent2D extent = {0, 0}, ScreenView const& view = {}) const noexcept;
	bool supported(Vsync vsync) const noexcept { return m_swapchain.supportedVsync().test(vsync); }

	ARenderer& renderer() const noexcept { return *m_storage.renderer; }
	CommandPool const& commandPool() const noexcept { return m_pool; }

  private:
	void initRenderer();

	template <typename... T>
	bool check(T... any) noexcept {
		return ((m_storage.status == any) || ...);
	}
	void set(Status s) noexcept { m_storage.status = s; }
	enum Flag { eRecreate = 1 << 0, eVsync = 1 << 1 };
	struct Storage {
		std::unique_ptr<ARenderer> renderer;
		std::optional<RenderTarget> drawing;
		Deferred<vk::PipelineCache> pipelineCache;
		Status status = {};
		struct {
			std::optional<graphics::Vsync> vsync;
			bool trigger = false;
		} reconstruct;
	};

	Swapchain m_swapchain;
	Storage m_storage;
	CommandPool m_pool;
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

template <typename T, typename... Args>
void RenderContext::makeRenderer(Args&&... args) {
	ENSURE(m_storage.status < Status::eReady, "Invalid RenderContext status");
	m_storage.renderer = std::make_unique<T>(&m_swapchain, std::forward<Args>(args)...);
	initRenderer();
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
