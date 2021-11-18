#pragma once
#include <core/lib_logger.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>

namespace le::graphics {
struct Bootstrap : Pinned {
	using MakeSurface = graphics::Device::MakeSurface;
	struct CreateInfo;

	Bootstrap(CreateInfo const& info, Device::MakeSurface&& makeSurface);
	~Bootstrap();

	Device device;
	VRAM vram;
};

struct Bootstrap::CreateInfo {
	Device::CreateInfo device;
	Transfer::CreateInfo transfer;
	LibLogger::Verbosity verbosity = LibLogger::libVerbosity;
};
} // namespace le::graphics
