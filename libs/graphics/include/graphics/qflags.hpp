#pragma once
#include <core/std_types.hpp>
#include <kt/enum_flags/enum_flags.hpp>

namespace le::graphics {
enum class QType { eGraphics, ePresent, eTransfer, eCOUNT_ };
using QFlags = kt::enum_flags<QType, u8>;
constexpr QFlags qflags_all = QFlags(QType::eGraphics, QType::ePresent, QType::eTransfer);

constexpr auto q0 = QFlags::make(QType::eGraphics);
constexpr auto q01 = q0.test(QType::eGraphics);
constexpr auto q = qflags_all.test(QType::eGraphics);
static_assert(qflags_all.count() == std::size_t(QType::eCOUNT_), "Invariant violated");
} // namespace le::graphics
