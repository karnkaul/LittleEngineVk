#pragma once
#include <core/std_types.hpp>
#include <ktl/enum_flags/enum_flags.hpp>

namespace le::graphics {
enum class QType { eNone, eGraphics, eCompute };
using QCaps = ktl::enum_flags<QType, u8>;
} // namespace le::graphics
