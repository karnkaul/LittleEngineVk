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

enum class PFlag { eDepthTest, eDepthWrite, eCOUNT_ };
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

	using MinMag = std::pair<vk::Filter, vk::Filter>;

	static VertexInputInfo vertexInput(VertexInputCreateInfo const& info);
	static VertexInputInfo vertexInput(QuickVertexInput const& info);
	template <typename V = Vertex>
	static Pipeline::CreateInfo pipeInfo(Shader const& shader, PFlags flags = PFlag::eDepthTest | PFlag::eDepthWrite);
	static vk::SamplerCreateInfo samplerInfo(MinMag minMag, vk::SamplerMipmapMode mip = vk::SamplerMipmapMode::eLinear);

	RenderContext(Swapchain& swapchain, u32 rotateCount = 2, u32 secondaryCmdCount = 0);
	RenderContext(RenderContext&&);
	RenderContext& operator=(RenderContext&&);
	~RenderContext();

	bool waitForFrame();
	std::optional<Frame> beginFrame(CommandBuffer::PassInfo const& info);
	bool endFrame();

	std::optional<Render> render(Colour clear = colours::black, vk::ClearDepthStencilValue depth = {1.0f, 0});

	Status status() const noexcept;
	std::size_t index() const noexcept;
	bool reconstructd(glm::ivec2 framebufferSize);

	View<Pipeline> makePipeline(std::string_view id, Pipeline::CreateInfo createInfo);
	View<Pipeline> pipeline(Hash id);
	bool destroyPipeline(Hash id);
	bool hasPipeline(Hash id) const noexcept;

	vk::Sampler makeSampler(vk::SamplerCreateInfo const& info);

	vk::Viewport viewport(vk::Extent2D extent = {0, 0}, glm::vec2 const& depth = {0.0f, 1.0f}, ScreenRect const& nRect = {}) const noexcept;
	vk::Rect2D scissor(vk::Extent2D extent = {0, 0}, ScreenRect const& nRect = {}) const noexcept;

  private:
	void destroy();

	struct Storage {
		std::unordered_map<Hash, Pipeline> pipes;
		std::vector<vk::Sampler> samplers;
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
	u32 binding = 0;
	std::size_t size = 0;
	std::vector<std::size_t> offsets;
};

// impl

template <typename V>
Pipeline::CreateInfo RenderContext::pipeInfo(Shader const& shader, PFlags flags) {
	Pipeline::CreateInfo ret;
	ret.dynamicState.shader = shader;
	ret.fixedState.vertexInput = VertexInfoFactory<V>()(0);
	if (flags.test(PFlag::eDepthTest)) {
		ret.fixedState.depthStencilState.depthTestEnable = true;
		ret.fixedState.depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	}
	if (flags.test(PFlag::eDepthWrite)) {
		ret.fixedState.depthStencilState.depthWriteEnable = true;
	}
	return ret;
}

inline std::size_t RenderContext::index() const noexcept {
	return m_sync.index;
}
inline RenderContext::Status RenderContext::status() const noexcept {
	return m_storage.status;
}
} // namespace le::graphics
