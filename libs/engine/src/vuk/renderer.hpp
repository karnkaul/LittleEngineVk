#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "core/delegate.hpp"
#include "common.hpp"
#include "utils.hpp"

namespace le::vuk
{
struct UniformData
{
	VkResource<vk::Buffer> buffer;
	vk::DescriptorSet descriptorSet;
	vk::DeviceSize offset;
};

class Renderer final
{
public:
	struct FrameDriver final
	{
		struct UBOs
		{
			UniformData view;
		};

		UBOs ubos;
		vk::CommandBuffer commandBuffer;
	};

	using Write = std::function<void(FrameDriver::UBOs const&)>;
	using Draw = std::function<void(FrameDriver const&)>;

private:
	struct FrameSync final
	{
		struct
		{
			UniformData view;
		} ubos;
		vk::Semaphore renderReady;
		vk::Semaphore presentReady;
		vk::Fence drawing;
		vk::CommandBuffer commandBuffer;
		vk::CommandPool commandPool;
		bool bNascent = true;
	};

	struct Shared final
	{
		UBOData uboView;
	};

public:
	static std::string const s_tName;

private:
	static Shared const s_shared;

public:
	vk::DescriptorPool m_descriptorPool;

private:
	std::vector<FrameSync> m_frames;
	size_t m_index = 0;
	std::vector<Delegate<>::Token> m_callbackTokens;
	class Presenter* m_pPresenter = nullptr;
	WindowID m_window;
	u8 m_frameCount = 0;

public:
	Renderer(Presenter* pPresenter, u8 frameCount);
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
} // namespace le::vuk
