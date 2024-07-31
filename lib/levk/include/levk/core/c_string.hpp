#pragma once
#include <string_view>

namespace levk {
/// \brief Wrapper over a C string.
/// Never points to nullptr.
class CString {
  public:
	/*implicit*/ constexpr CString(char const* str = "") : m_str(str == nullptr ? "" : str) {}

	[[nodiscard]] constexpr auto c_str() const -> char const* { return m_str; }
	[[nodiscard]] constexpr auto as_view() const -> std::string_view { return c_str(); }

  private:
	char const* m_str;
};
} // namespace levk
