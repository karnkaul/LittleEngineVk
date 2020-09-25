#pragma once
#include <deque>
#include <vector>
#include <glm/glm.hpp>
#include <core/delegate.hpp>
#include <engine/window/common.hpp>
#include <engine/gfx/light.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/gfx/render_driver.hpp>
#include <gfx/render_context.hpp>
#include <gfx/pipeline_impl.hpp>
#include <gfx/resource_descriptors.hpp>

namespace le
{
class Transform;
class WindowImpl;
} // namespace le

namespace le::gfx::render
{
struct FrameSync final
{
	rd::Set set;
	vk::Semaphore renderReady;
	vk::Semaphore presentReady;
	vk::Fence drawing;
	vk::Framebuffer framebuffer;
	vk::CommandBuffer commandBuffer;
	vk::CommandPool commandPool;
};

struct TexCounts
{
	u32 diffuse = 0;
	u32 specular = 0;
};

using PCDeq = std::deque<std::deque<rd::PushConstants>>;

class Driver::Impl
{
public:
	struct Info final
	{
		ContextInfo contextInfo;
		WindowID windowID;
		u8 frameCount = 3;
	};

public:
	std::string m_name;

private:
	RenderContext m_context;
	RenderPass m_renderPass;
	std::vector<FrameSync> m_frames;
	Driver* m_pDriver;
	TexCounts m_texCount;

	u64 m_drawnFrames = 0;

	std::size_t m_index = 0;
	WindowID m_window;
	u8 m_frameCount = 0;

public:
	Impl(Info const& info, Driver* pOwner);
	~Impl();

public:
	void create(u8 frameCount = 2);
	void destroy();

	void update();
	bool render(Driver::Scene scene, bool bExtGUI);

public:
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

	friend class le::WindowImpl;
};

struct PassInfo final
{
	Ref<RenderContext const> context;
	Ref<FrameSync const> frame;
	Ref<Driver::Scene const> scene;
	Ref<PCDeq const> push;
	Ref<RenderTarget const> target;
	RenderPass pass;
	ScreenRect view;
	bool bExtGUI = false;
};

PCDeq write(FrameSync& out_frame, Driver::Scene& out_scene, TexCounts& out_texCount);
u64 pass(PassInfo const& info);
bool submit(RenderContext& out_context, FrameSync const& frame);
vk::Viewport viewport(vk::Extent2D extent, ScreenRect const& nRect = {}, glm::vec2 const& depth = {0.0f, 1.0f});
vk::Rect2D scissor(vk::Extent2D extent, ScreenRect const& nRect = {});
} // namespace le::gfx::render
