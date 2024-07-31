#pragma once
#include <imgui.h>
#include <levk/core/c_string.hpp>
#include <levk/core/polymorphic.hpp>
#include <span>
#include <string_view>
#include <vector>

namespace levk {
/// \brief Expandable ImGui input text buffer.
class ImInputText : public Polymorphic {
  public:
	static constexpr std::size_t init_size_v{64};

	virtual auto update(CString name) -> bool;
	void set_text(std::string_view text);

	auto operator()(CString name) -> bool { return update(name); }

	[[nodiscard]] auto as_view() const -> std::string_view { return m_buffer.data(); }
	[[nodiscard]] auto as_span() const -> std::span<char const> { return m_buffer; }

	int flags{};

  protected:
	virtual auto on_callback(ImGuiInputTextCallbackData& data) -> int;

	void resize_buffer(ImGuiInputTextCallbackData& data);

	std::vector<char> m_buffer{};
};

/// \brief Expandable ImGui multi-line input text buffer.
class ImInputTextMultiLine : public ImInputText {
  public:
	auto update(CString name) -> bool override;

	ImVec2 size{};
};
} // namespace levk
