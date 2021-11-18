#pragma once
#include <core/lib_logger.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/swapchain.hpp>

namespace le::graphics {
struct Bootstrap : Pinned {
	using MakeSurface = graphics::Device::MakeSurface;
	struct CreateInfo;

	Bootstrap(CreateInfo const& info, Device::MakeSurface&& makeSurface, glm::ivec2 framebufferSize = {});
	~Bootstrap();

	Device device;
	VRAM vram;
	Swapchain swapchain;
};

struct Bootstrap::CreateInfo {
	Device::CreateInfo device;
	Transfer::CreateInfo transfer;
	Swapchain::CreateInfo swapchain;
	LibLogger::Verbosity verbosity = LibLogger::libVerbosity;
};
} // namespace le::graphics
