#pragma once
#include <core/std_types.hpp>
#include <ktl/enum_flags/enum_flags.hpp>

namespace le::graphics {
enum class QType { eGraphics, ePresent, eTransfer, eCOUNT_ };
using QFlags = ktl::enum_flags<QType, u8>;
constexpr QFlags qflags_all = QFlags(QType::eGraphics, QType::ePresent, QType::eTransfer);

static_assert(qflags_all.count() == std::size_t(QType::eCOUNT_), "Invariant violated");
} // namespace le::graphics
