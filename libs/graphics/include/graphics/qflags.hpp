#pragma once
#include <core/std_types.hpp>
#include <ktl/enum_flags/enum_flags.hpp>

namespace le::graphics {
enum class QFlag { eGraphics, ePresent, eTransfer, eCompute, eCOUNT_ };
using QFlags = ktl::enum_flags<QFlag, u8>;
} // namespace le::graphics
