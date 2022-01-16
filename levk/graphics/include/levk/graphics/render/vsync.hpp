#pragma once
#include <levk/core/std_types.hpp>
#include <string_view>

namespace le::graphics {
enum class VSync : u8 { eOn, eAdaptive, eOff, eCOUNT_ };
constexpr EnumArray<VSync, std::string_view> vSyncNames = {"Vsync On", "Vsync Adaptive", "Vsync Off"};
} // namespace le::graphics
