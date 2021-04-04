#pragma once
#include <string_view>
#include <unordered_map>
#include <core/colour.hpp>
#include <core/hash.hpp>
#include <graphics/context/frame_sync.hpp>
#include <graphics/context/swapchain.hpp>
#include <graphics/geometry.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/screen_rect.hpp>

namespace le::graphics {

struct QuickVertexInput;
struct VertexInputCreateInfo;

enum class PFlag { eDepthTest, eDepthWrite, eAlphaBlend, eCOUNT_ };
using PFlags = kt::enum_flags<PFlag>;

class RenderContext : NoCopy {
  public:
	enum class Status { eIdle, eWaiting, eReady, eDrawing };
	struct Frame {
		RenderTarget target;
		CommandBuffer primary;
	};
	class Render {
	  public:
		Render(RenderContext& context, Frame&& frame);
		Render(Render&&);
		Render& operator=(Render&&);
		~Render();

		RenderTarget const& target() const {
			return m_frame.target;
		}
		CommandBuffer& primary() {
			return m_frame.primary;
		}

		Frame m_frame;

	  private:
		void destroy();

		vk::Semaphore m_presentReady;
		vk::Fence m_drawing;
		Ref<RenderContext> m_context;
	};

	static VertexInputInfo vertexInput(VertexInputCreateInfo const& info);
	static VertexInputInfo vertexInput(QuickVertexInput const& info);
	template <typename V = Vertex>
	static Pipeline::CreateInfo pipeInfo(PFlags flags = PFlags(PFlag::eDepthTest) | PFlag::eDepthWrite);

	RenderContext(Swapchain& swapchain, u32 rotateCount = 2, u32 secondaryCmdCount = 0);
	RenderContext(RenderContext&&);
	RenderContext& operator=(RenderContext&&);

	bool waitForFrame();
	std::optional<Frame> beginFrame(CommandBuffer::PassInfo const& info);
	bool endFrame();

	std::optional<Render> render(Colour clear = colours::black, vk::ClearDepthStencilValue depth = {1.0f, 0});

	Status status() const noexcept;
	std::size_t index() const noexcept;
	std::size_t rotateCount() const noexcept;
	glm::ivec2 extent() const noexcept;
	bool reconstructed(glm::ivec2 framebufferSize);

	Pipeline makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo createInfo);

	vk::SurfaceFormatKHR swapchainFormat() const noexcept;
	vk::Format textureFormat() const noexcept;

	f32 aspectRatio() const noexcept;
	glm::mat4 preRotate() const noexcept;
	vk::Viewport viewport(glm::ivec2 extent = {0, 0}, glm::vec2 depth = {0.0f, 1.0f}, ScreenRect const& nRect = {}, glm::vec2 offset = {}) const noexcept;
	vk::Rect2D scissor(glm::ivec2 extent = {0, 0}, ScreenRect const& nRect = {}) const noexcept;

  private:
	struct Storage {
		std::optional<RenderTarget> target;
		Status status = {};
	};

	BufferedFrameSync m_sync;
	Storage m_storage;

	Ref<Swapchain> m_swapchain;
	Ref<VRAM> m_vram;
	Ref<Device> m_device;
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
	if (flags.test(PFlag::eDepthWrite)) {
		ret.fixedState.depthStencilState.depthWriteEnable = true;
	}
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

inline std::size_t RenderContext::index() const noexcept {
	return m_sync.index;
}
inline std::size_t RenderContext::rotateCount() const noexcept {
	return m_sync.size();
}
inline RenderContext::Status RenderContext::status() const noexcept {
	return m_storage.status;
}
inline glm::ivec2 RenderContext::extent() const noexcept {
	vk::Extent2D const ext = m_swapchain.get().display().extent;
	return glm::ivec2(ext.width, ext.height);
}
inline vk::SurfaceFormatKHR RenderContext::swapchainFormat() const noexcept {
	return m_swapchain.get().colourFormat();
}
inline vk::Format RenderContext::textureFormat() const noexcept {
	return swapchainFormat().colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Snorm;
}
} // namespace le::graphics
