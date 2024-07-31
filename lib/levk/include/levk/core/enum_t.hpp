#pragma once
#include <type_traits>

namespace levk {
/// \brief Concept for enums.
template <typename Type>
concept EnumT = std::is_enum_v<Type>;
} // namespace levk
