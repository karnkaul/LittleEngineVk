#pragma once
#include <format>
#include <stdexcept>

namespace levk {
/// \brief Library Exception type.
class Error : public std::runtime_error {
  public:
	template <typename... Args>
	explicit Error(std::format_string<Args...> fmt, Args&&... args) : std::runtime_error(std::format(fmt, std::forward<Args>(args)...)) {}
};
} // namespace levk
