#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "core/delegate.hpp"
#include "draw/resource_descriptors.hpp"

namespace le::gfx
{
class Renderer final
{
public:
	struct Info final
	{
		Presenter* pPresenter = nullptr;
		u8 frameCount = 2;
	};

	struct FrameDriver final
	{
		rd::Handles descriptorHandles;
		vk::CommandBuffer commandBuffer;
	};

	using Write = std::function<void(rd::Handles const&)>;
	using Draw = std::function<void(FrameDriver const&)>;

private:
	struct FrameSync final
	{
		rd::Handles descriptorHandles;
		vk::Semaphore renderReady;
		vk::Semaphore presentReady;
		vk::Fence drawing;
		vk::Framebuffer framebuffer;
		vk::CommandBuffer commandBuffer;
		vk::CommandPool commandPool;
		bool bNascent = true;
	};

public:
	static std::string const s_tName;

private:
	vk::DescriptorPool m_descriptorPool;
	std::vector<FrameSync> m_frames;

	size_t m_index = 0;
	std::vector<Delegate<>::Token> m_callbackTokens;
	class Presenter* m_pPresenter = nullptr;
	WindowID m_window;
	u8 m_frameCount = 0;

public:
	Renderer(Info const& info);
	~Renderer();

public:
	void create(u8 frameCount = 2);
	void destroy();
	void reset();

	bool render(Write write, Draw draw, ClearValues const& clear);

public:
	vk::Viewport transformViewport(ScreenRect const& nRect = {}, glm::vec2 const& depth = {0.0f, 1.0f}) const;
	vk::Rect2D transformScissor(ScreenRect const& nRect = {}) const;

private:
	FrameSync& frameSync();
	void next();
};
} // namespace le::gfx
