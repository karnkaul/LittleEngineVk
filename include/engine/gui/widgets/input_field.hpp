#pragma once
#include <engine/gui/widget.hpp>
#include <engine/input/receiver.hpp>
#include <engine/input/text_cursor.hpp>

namespace le::gui {
class InputField : public Widget, public input::Receiver {
  public:
	struct CreateInfo {
		glm::vec2 size = {200.0f, 50.0f};
		f32 border = 5.0f;
		bool active = false;
		bool multiLine = false;
	};

	InputField(not_null<TreeRoot*> root, not_null<BitmapFont const*> font, CreateInfo const& info, Hash style = {});

	InputField(InputField&&) = delete;
	InputField& operator=(InputField&&) = delete;

	Status onInput(input::State const& state) override;
	Span<Prop const> props() const noexcept override;

	bool block(input::State const& state) override;

	void setActive(bool active) noexcept;
	InputField& align(glm::vec2 align) noexcept;

  protected:
	TextMesh m_mesh;
	input::TextCursor m_cursor;

  private:
	mutable ktl::fixed_vector<Prop, 4> m_props;
};

// impl

inline InputField& InputField::align(glm::vec2 align) noexcept {
	m_cursor.m_gen.align = align;
	m_cursor.m_gen.position = {m_rect.size * m_cursor.m_gen.align, m_zIndex};
	m_cursor.refresh();
	return *this;
}
} // namespace le::gui
