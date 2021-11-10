#pragma once
#include <graphics/render/renderer.hpp>

namespace le::graphics {
namespace rtech {
// constexpr Tech fwdSwpRp = {Approach::eForward, Target::eSwapchain, Transition::eRenderPass};
// constexpr Tech fwdSwpCb = {Approach::eForward, Target::eSwapchain, Transition::eCommandBuffer};
// constexpr Tech fwdOffCb = {Approach::eForward, Target::eOffScreen, Transition::eCommandBuffer};
enum Tech_ {
	fwdSwpRp,
	fwdSwpCb,
	fwdOffCb,
};
} // namespace rtech

// Forward, Swapchain, RenderPass
class RendererFSR;
// Forward, Swapchain, CommandBuffer
class RendererFSC;
// Forward, OffScreen, CommandBuffer
class RendererFOC;

template <rtech::Tech_ RT>
struct Renderer_type;
template <rtech::Tech_ RT>
using Renderer_t = typename Renderer_type<RT>::type;

class RendererFSR : public ARenderer {
  public:
	RendererFSR(not_null<Swapchain*> swapchain, Buffering buffering = doubleBuffer);

	static constexpr Tech tech_v = {Approach::eForward, Target::eSwapchain, Transition::eRenderPass};
	Tech tech() const noexcept override { return tech_v; }

	CommandBuffer beginDraw(RenderTarget const& target, ScreenView const& view, RGBA clear, ClearDepth depth) override;
	void endDraw(RenderTarget const& target) override;
};

class RendererFSC : public RendererFSR {
  public:
	RendererFSC(not_null<Swapchain*> swapchain, Buffering buffering = doubleBuffer);

	static constexpr Tech tech_v = {Approach::eForward, Target::eSwapchain, Transition::eCommandBuffer};
	Tech tech() const noexcept override { return tech_v; }

	CommandBuffer beginDraw(RenderTarget const& target, ScreenView const& view, RGBA clear, ClearDepth depth) override;
	void endDraw(RenderTarget const& target) override;
};

class RendererFOC : public RendererFSR {
  public:
	RendererFOC(not_null<Swapchain*> swapchain, Buffering buffering = doubleBuffer);

	static constexpr Tech tech_v = {Approach::eForward, Target::eOffScreen, Transition::eCommandBuffer};
	Tech tech() const noexcept override { return tech_v; }

	CommandBuffer beginDraw(RenderTarget const& target, ScreenView const& view, RGBA clear, ClearDepth depth) override;
	std::optional<RenderTarget> beginFrame() override;
	void endDraw(RenderTarget const& target) override;

  private:
	RenderImage m_swapchainImage;
	vk::Format m_colourFormat = vk::Format::eR8G8B8A8Unorm;
	std::size_t m_colourIndex = 0;
};

template <>
struct Renderer_type<rtech::fwdSwpRp> {
	using type = RendererFSR;
};
template <>
struct Renderer_type<rtech::fwdSwpCb> {
	using type = RendererFSC;
};
template <>
struct Renderer_type<rtech::fwdOffCb> {
	using type = RendererFOC;
};
} // namespace le::graphics
