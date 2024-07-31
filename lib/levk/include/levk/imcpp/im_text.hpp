#pragma once
#include <imgui.h>
#include <levk/core/fixed_string.hpp>

namespace levk {
/// \brief ImGui::Text wrapper that uses FixedString (no allocations).
template <typename... Args>
void im_text(std::format_string<Args...> fmt, Args&&... args) {
	static constexpr auto buffer_size_v = 256;
	static auto s_buffer = FixedString<buffer_size_v>{};
	s_buffer = FixedString<buffer_size_v>{fmt, std::forward<Args>(args)...};
	ImGui::Text("%s", s_buffer.c_str()); // NOLINT(cppcoreguidelines-pro-type-vararg)
}
} // namespace levk
