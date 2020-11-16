#pragma once
#include <functional>
#include <graphics/context/device.hpp>
#include <graphics/context/instance.hpp>
#include <graphics/context/swapchain.hpp>
#include <graphics/context/vram.hpp>

namespace le::graphics {
struct Bootstrap {
	using MakeSurface = std::function<vk::SurfaceKHR(vk::Instance)>;
	struct CreateInfo;

	Bootstrap(CreateInfo const& info, MakeSurface const& makeSuface, glm::ivec2 framebufferSize);

	Instance instance;
	Device device;
	VRAM vram;
	Swapchain swapchain;
};

struct Bootstrap::CreateInfo {
	Instance::CreateInfo instance;
	Device::CreateInfo device;
	Transfer::CreateInfo transfer;
	Swapchain::CreateInfo swapchain;
};

// impl

inline Bootstrap::Bootstrap(CreateInfo const& info, MakeSurface const& makeSurface, glm::ivec2 framebufferSize)
	: instance(info.instance), device(instance, makeSurface(instance.m_instance), info.device), vram(device, info.transfer),
	  swapchain(vram, framebufferSize, info.swapchain) {
	logD("[{}] Vulkan bootstrapped", g_name);
}
} // namespace le::graphics
