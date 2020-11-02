#pragma once
#include <core/std_types.hpp>
#include <engine/window/common.hpp>
#include <vulkan/vulkan.hpp>

namespace le::gfx::ext_gui {
struct Info final {
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
} // namespace le::gfx::ext_gui
