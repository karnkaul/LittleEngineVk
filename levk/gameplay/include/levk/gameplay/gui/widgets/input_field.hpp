#pragma once
#include <levk/engine/input/receiver.hpp>
#include <levk/engine/input/text_cursor.hpp>
#include <levk/gameplay/gui/widget.hpp>
#include <levk/graphics/font/font.hpp>

namespace le::gui {
class InputField : public Widget, public input::Receiver {
  public:
	using Font = graphics::Font;
	struct CreateInfo;

	InputField(not_null<TreeRoot*> root, CreateInfo const& info, Hash fontURI = defaultFontURI, Hash style = {});

	InputField(InputField&&) = delete;
	InputField& operator=(InputField&&) = delete;

	Status onInput(input::State const& state) override;
	MeshView mesh() const noexcept override;
	void addDrawPrimitives(DrawList& out) const override;

	bool block(input::State const& state) override;

	void setActive(bool active) noexcept;
	InputField& align(Font::Align horz) noexcept;
	std::string_view text() const noexcept { return m_secret ? m_exposed : m_cursor.m_line; }

  protected:
	Hash m_fontURI;
	TextMesh m_textMesh;
	input::TextCursor m_cursor;

  private:
	void onUpdate(input::Space const& space) override;
	void reposition() noexcept;

	mutable ktl::fixed_vector<MeshObj, 4> m_meshes;
	std::string m_exposed;
	std::optional<Quad> m_outline;
	f32 m_offsetX{};
	bool m_secret{};
};

struct InputField::CreateInfo {
	glm::vec2 size = {200.0f, 50.0f};
	f32 border = 5.0f;
	f32 offsetX = 10.0f;
	f32 alpha = 0.85f;
	bool active = false;
	bool secret = false;
};

// impl

inline InputField& InputField::align(Font::Align const horz) noexcept {
	m_cursor.m_layout.pivot = Font::pivot(horz, Font::Align::eMin);
	reposition();
	m_cursor.refresh();
	return *this;
}
} // namespace le::gui
