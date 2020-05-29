#pragma once
#include <deque>
#include <vector>
#include <glm/glm.hpp>
#include "core/delegate.hpp"
#include "engine/window/common.hpp"
#include "engine/gfx/light.hpp"
#include "engine/gfx/pipeline.hpp"
#include "engine/gfx/renderer.hpp"
#include "presenter.hpp"
#include "pipeline_impl.hpp"
#include "resource_descriptors.hpp"

namespace le
{
class Transform;
class WindowImpl;
} // namespace le

namespace le::gfx
{
class Mesh;
class Renderer;

class RendererImpl final
{
public:
	struct Info final
	{
		PresenterInfo presenterInfo;
		WindowID windowID;
		u8 frameCount = 3;
		bool bGUI = false;
	};

private:
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

	using PCDeq = std::deque<std::deque<rd::PushConstants>>;

public:
	std::string m_name;

private:
	Presenter m_presenter;
	std::deque<Pipeline> m_pipelines;
	std::vector<FrameSync> m_frames;
	vk::DescriptorSetLayout m_samplerLayout;
	Renderer* m_pRenderer;
	struct
	{
		Pipeline* pDefault = nullptr;
		Pipeline* pSkybox = nullptr;
	} m_pipes;

	struct
	{
		u32 diffuse = 0;
		u32 specular = 0;
	} m_texCount;

	u64 m_drawnFrames = 0;

	size_t m_index = 0;
	WindowID m_window;
	u8 m_frameCount = 0;
	bool m_bGUI = false;

public:
	RendererImpl(Info const& info, Renderer* pOwner);
	~RendererImpl();

public:
	void create(u8 frameCount = 2);
	void destroy();

	void update();

	Pipeline* createPipeline(Pipeline::Info info);
	bool render(Renderer::Scene scene);

public:
	vk::Viewport transformViewport(ScreenRect const& nRect = {}, glm::vec2 const& depth = {0.0f, 1.0f}) const;
	vk::Rect2D transformScissor(ScreenRect const& nRect = {}) const;

	u64 framesDrawn() const;
	u8 virtualFrameCount() const;

	glm::vec2 screenToN(glm::vec2 const& screenXY) const;
	ScreenRect clampToView(glm::vec2 const& screenXY, glm::vec2 const& nViewport, glm::vec2 const& padding = {}) const;

private:
	void onFramebufferResize();
	FrameSync& frameSync();
	void next();

	PCDeq writeSets(Renderer::Scene& out_scene, FrameSync& out_frame);
	u64 doRenderPass(FrameSync& out_frame, Renderer::Scene const& scene, Presenter::Pass const& pass, PCDeq const& push);
	Presenter::Outcome submit(FrameSync const& frame);

	friend class le::WindowImpl;
};
} // namespace le::gfx
