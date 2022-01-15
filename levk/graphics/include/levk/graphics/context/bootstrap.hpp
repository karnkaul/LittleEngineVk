#pragma once
#include <levk/graphics/context/device.hpp>
#include <levk/graphics/context/vram.hpp>

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
};
} // namespace le::graphics
