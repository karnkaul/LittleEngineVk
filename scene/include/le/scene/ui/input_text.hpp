#pragma once
#include <le/input/receiver.hpp>
#include <le/scene/ui/primitive_renderer.hpp>
#include <le/scene/ui/text.hpp>
#include <le/scene/ui/view.hpp>

namespace le::ui {
struct Cursor {
	static constexpr Duration blink_rate_v{1s};
	static constexpr glm::vec2 scale_v{0.1f, 1.0f};

	glm::vec2 scale{scale_v};
	float n_y_offset{0.4f};
	Duration blink_rate{blink_rate_v};
	std::size_t position{};
};

class InputText : public View, public input::Receiver {
  public:
	auto reset_blink() -> void;
	auto write(std::string_view str) -> void;
	auto backspace() -> void;
	auto delete_front() -> void;
	auto step_front() -> void;
	auto step_back() -> void;
	auto goto_home() -> void;
	auto goto_end() -> void;
	auto paste_clipboard() -> void;

	[[nodiscard]] auto get_text() const -> Text& { return *m_text; }

	Cursor cursor{};
	bool enabled{true};

  protected:
	auto setup() -> void override;
	auto tick(Duration dt) -> void override;

	auto on_key(int key, int action, int mods) -> bool override;
	auto on_char(input::Codepoint codepoint) -> bool override;

	auto update_cursor(Duration dt) -> void;

	Duration m_cursor_elapsed{};
	Ptr<PrimitiveRenderer> m_cursor{};
	Ptr<Text> m_text{};
};
} // namespace le::ui
