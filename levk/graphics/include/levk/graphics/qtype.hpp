#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <levk/core/std_types.hpp>

namespace le::graphics {
enum class QType { eNone, eGraphics, eCompute };
using QCaps = ktl::enum_flags<QType, u8>;
} // namespace le::graphics
