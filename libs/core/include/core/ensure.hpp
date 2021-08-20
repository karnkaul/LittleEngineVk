#pragma once
#include <string>

namespace le {
///
/// \brief Call utils::error if pred is false
///
void ensure(bool pred, std::string_view msg = {}, char const* fl = __builtin_FILE(), char const* fn = __builtin_FUNCTION(), int ln = __builtin_LINE());
} // namespace le
