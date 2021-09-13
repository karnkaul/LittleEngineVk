#pragma once
#include <engine/render/bitmap_text.hpp>

namespace le {
class BitmapFont;
}

namespace le::input {
struct State;

///
/// \brief Interactive text input with cursor prop
///
/// Regular calls to update() are required (preferably every frame) for consistent and lag-free response
///
class TextCursor {
  public:
	enum class Flag {
		// Ignore new lines
		eNoNewLine,
		// Do not control cursor blinking
		eNoAutoBlink,
		// Enable cursor and input
		eActive
	};
	using Flags = ktl::enum_flags<Flag>;

	static constexpr std::size_t npos = std::string_view::npos;

	TextCursor(not_null<BitmapFont const*> font, Flags flags = {});

	///
	/// \brief Obtain prop corresponding to cursor
	///
	Prop const& cursor() const noexcept;
	///
	/// \brief Obtain cursor if active else empty Span
	///
	Span<Prop const> props() const noexcept;

	///
	/// \brief Erase character one behind cursor and decrement cursor
	///
	/// Eg: index == 0: noop; 0 < index >= 1: text.erase(index-- - 1)
	///
	void backspace(graphics::Geometry* out = {});
	///
	/// \brief Erase character at cursor
	///
	/// Eg: 0 <= index < text.size: text.erase(index); index >= text.size: noop
	///
	void deleteFront(graphics::Geometry* out = {});
	///
	/// \brief Insert character at cursor and increment cursor
	///
	void insert(char ch, graphics::Geometry* out = {});
	///
	/// \brief Update input, and cursor (if eNoAutoBlink is not set)
	/// \returns true if refreshed
	///
	bool update(State const& state, graphics::Geometry* out = {});
	///
	/// \brief Refresh cursor
	///
	void refresh(graphics::Geometry* out = {});
	///
	/// \brief Generate text geometry
	///
	graphics::Geometry generateText();

	///
	/// \brief Check if cursor is active
	///
	bool active() const noexcept { return m_flags.test(Flag::eActive); }
	///
	/// \brief Set (in)active
	///
	void setActive(bool active) noexcept;
	///
	/// \brief Set percentage of (advanced, height) to offset cursor by
	///
	void offset(glm::vec2 nOffset) noexcept { m_offset = nOffset; }
	///
	/// \brief Set cursor index: will be placed behind character at index
	///
	/// Eg: 0 <= index < size: before glyph at index; index >= size: after last glyph
	///
	void index(std::size_t index);
	///
	/// \brief Get cursor index
	///
	std::size_t index() const noexcept { return m_index; }
	///
	/// \brief Set whether to draw cursor (may be overridden on update() unless eNoAutoBlink is set)
	///
	void drawCursor(bool draw) noexcept { m_drawCursor = draw; }
	///
	/// \brief Set normalized cursor width and height (scaled to font extent)
	///
	void cursorSize(glm::vec2 nSize) { m_size = nSize; }
	///
	/// \brief Obtain cursor size
	///
	glm::vec2 cursorSize() const noexcept { return m_size; }

	std::string m_text;
	TextGen m_gen;
	Flags m_flags;
	Time_s m_blinkRate = 500ms;
	f32 m_alpha = 0.85f;

  private:
	graphics::Mesh m_mesh;
	mutable Prop m_prop;
	glm::vec3 m_position{};
	glm::vec2 m_offset = {0.3f, 0.3f};
	glm::vec2 m_size = {0.1f, 1.1f};
	time::Point m_lastBlink;
	std::size_t m_index = npos;
	not_null<BitmapFont const*> m_font;
	bool m_drawCursor = true;
};
} // namespace le::input
