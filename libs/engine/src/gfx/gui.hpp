#pragma once
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"
#include "engine/window/common.hpp"

namespace le::gfx::gui
{
struct Info final
{
	vk::RenderPass renderPass;
	WindowID window;
	u8 imageCount = 3;
	u8 minImageCount = 2;
};

bool init(Info const& info);
void deinit();

void newFrame();
void renderDrawData(vk::CommandBuffer commandBuffer);
void render();

bool isInit();
} // namespace le::gfx::gui
