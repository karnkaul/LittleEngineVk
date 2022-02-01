#pragma once
#include <ft/ft.hpp>
#include <levk/graphics/device/device.hpp>

namespace le::graphics {
struct Device::Impl {
	FTUnique<FTLib> ftLib;
};
} // namespace le::graphics
