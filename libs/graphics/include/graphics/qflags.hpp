#pragma once
#include <kt/enum_flags/enum_flags.hpp>

namespace le::graphics {
enum class QType { eGraphics, ePresent, eTransfer, eCOUNT_ };
using QFlags = kt::enum_flags<QType>;
} // namespace le::graphics
