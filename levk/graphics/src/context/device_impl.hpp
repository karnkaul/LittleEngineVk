#pragma once
#include <ft/ft.hpp>
#include <levk/graphics/context/device.hpp>

namespace le::graphics {
struct Device::Impl {
	FTUnique<FTLib> ftLib;
};
} // namespace le::graphics
