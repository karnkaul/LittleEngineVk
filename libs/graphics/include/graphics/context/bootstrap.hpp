#pragma once
#include <functional>
#include <core/lib_logger.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/instance.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/swapchain.hpp>

namespace le::graphics {
struct Bootstrap {
	using MakeSurface = std::function<vk::SurfaceKHR(vk::Instance)>;
	struct CreateInfo;

	Bootstrap(CreateInfo const& info, MakeSurface const& makeSuface, glm::ivec2 framebufferSize = {});

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
	LibLogger::Verbosity verbosity = LibLogger::libVerbosity;
};
} // namespace le::graphics
