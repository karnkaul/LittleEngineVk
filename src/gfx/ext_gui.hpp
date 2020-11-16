#pragma once
#include <core/erased_ref.hpp>
#include <core/std_types.hpp>
#include <vulkan/vulkan.hpp>

namespace le::gfx::ext_gui {
struct Info final {
	vk::RenderPass renderPass;
	ErasedRef glfwWindow;
	u8 imageCount = 3;
	u8 minImageCount = 2;
};

bool init(Info const& info);
void deinit();

void newFrame();
void renderDrawData(vk::CommandBuffer commandBuffer);
void render();

bool isInit();
} // namespace le::gfx::ext_gui
