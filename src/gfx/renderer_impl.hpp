#pragma once
#include <deque>
#include <vector>
#include <glm/glm.hpp>
#include <core/delegate.hpp>
#include <engine/window/common.hpp>
#include <engine/gfx/light.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/gfx/renderer.hpp>
#include <gfx/render_context.hpp>
#include <gfx/pipeline_impl.hpp>
#include <gfx/resource_descriptors.hpp>

namespace le
{
class Transform;
class WindowImpl;
} // namespace le

namespace le::gfx
{
class Renderer;

class RendererImpl final
{
public:
	struct Info final
	{
		ContextInfo contextInfo;
		WindowID windowID;
		u8 frameCount = 3;
	};

private:
	struct FrameSync;

	using PCDeq = std::deque<std::deque<rd::PushConstants>>;

public:
	std::string m_name;

private:
	RenderContext m_context;
	RenderPass m_renderPass;
	std::vector<FrameSync> m_frames;
	Renderer* m_pRenderer;

	struct
	{
		u32 diffuse = 0;
		u32 specular = 0;
	} m_texCount;

	u64 m_drawnFrames = 0;

	std::size_t m_index = 0;
	WindowID m_window;
	u8 m_frameCount = 0;

public:
	RendererImpl(Info const& info, Renderer* pOwner);
	~RendererImpl();

public:
	void create(u8 frameCount = 2);
	void destroy();

	void update();
	bool render(Renderer::Scene scene, bool bExtGUI);

public:
	vk::Viewport transformViewport(ScreenRect const& nRect = {}, glm::vec2 const& depth = {0.0f, 1.0f}) const;
	vk::Rect2D transformScissor(ScreenRect const& nRect = {}) const;

	u64 framesDrawn() const;
	u8 virtualFrameCount() const;

	glm::vec2 screenToN(glm::vec2 const& screenXY) const;
	ScreenRect clampToView(glm::vec2 const& screenXY, glm::vec2 const& nViewport, glm::vec2 const& padding = {}) const;
	bool initExtGUI() const;

	ColourSpace colourSpace() const;

	vk::PresentModeKHR presentMode() const;
	std::vector<vk::PresentModeKHR> const& presentModes() const;
	bool setPresentMode(vk::PresentModeKHR mode);

private:
	void onFramebufferResize();
	FrameSync& frameSync();
	FrameSync const& frameSync() const;
	void next();

	PCDeq writeSets(Renderer::Scene& out_scene);
	u64 doRenderPass(Renderer::Scene const& scene, PCDeq const& push, RenderTarget const& target, bool bExtGUI) const;
	bool submit();

	friend class le::WindowImpl;
};

struct RendererImpl::FrameSync final
{
	rd::Set set;
	vk::Semaphore renderReady;
	vk::Semaphore presentReady;
	vk::Fence drawing;
	vk::Framebuffer framebuffer;
	vk::CommandBuffer commandBuffer;
	vk::CommandPool commandPool;
};
} // namespace le::gfx
