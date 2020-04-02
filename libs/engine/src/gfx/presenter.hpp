#pragma once
#include <functional>
#include <vector>
#include <glm/glm.hpp>
#include "core/delegate.hpp"
#include "core/flags.hpp"
#include "engine/window/common.hpp"
#include "common.hpp"

namespace le::gfx
{
class Presenter final
{
public:
	enum class Flag
	{
		eRenderPaused = 0,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;
	using OnEvent = Delegate<>;

	struct DrawFrame final
	{
		vk::RenderPass renderPass;
		vk::Extent2D swapchainExtent;
		std::vector<vk::ImageView> attachments;
	};

private:
	struct Info final
	{
		PresenterInfo info;
		vk::SurfaceKHR surface;

		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> colourFormats;
		std::vector<vk::PresentModeKHR> presentModes;

		vk::SurfaceFormatKHR colourFormat;
		vk::Format depthFormat;
		vk::PresentModeKHR presentMode;

		void refresh();
		bool isReady() const;
		vk::SurfaceFormatKHR bestColourFormat() const;
		vk::Format bestDepthFormat() const;
		vk::PresentModeKHR bestPresentMode() const;
		vk::Extent2D extent(glm::ivec2 const& windowSize) const;
	};

	struct Swapchain final
	{
		struct Frame final
		{
			vk::ImageView colour;
			vk::ImageView depth;
			vk::Fence drawing;

			vk::Image image;
			bool bNascent = true;
		};

		Image depthImage;
		vk::ImageView depthImageView;
		vk::SwapchainKHR swapchain;
		std::vector<Frame> frames;

		vk::Extent2D extent;
		u32 imageIndex = 0;
		u8 imageCount = 0;

		Frame& frame();
	};

public:
	static std::string const s_tName;
	std::string m_name;

public:
	vk::RenderPass m_renderPass;
	Flags m_flags;

private:
	Info m_info;
	Swapchain m_swapchain;
	OnEvent m_onSwapchainRecreated;
	OnEvent m_onDestroyed;
	WindowID m_window;

public:
	Presenter(PresenterInfo const& info);
	~Presenter();

public:
	void onFramebufferResize();

	TResult<DrawFrame> acquireNextImage(vk::Semaphore setDrawReady, vk::Fence setDrawing = vk::Fence());
	bool present(vk::Semaphore wait);

	OnEvent::Token registerSwapchainRecreated(OnEvent::Callback callback);
	OnEvent::Token registerDestroyed(OnEvent::Callback callback);

private:
	void createRenderPass();
	bool createSwapchain();
	void destroySwapchain();
	void cleanup();

	bool recreateSwapchain();

private:
	friend class Renderer;
};
} // namespace le::gfx
