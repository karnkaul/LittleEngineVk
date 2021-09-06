#pragma once
#include <engine/input/receiver.hpp>
#include <engine/render/bitmap_text.hpp>

namespace le::input {
struct State;

///
/// \brief BitmapText with an interactive cursor
///
/// Maintains an additional mesh with a cursor, switches between base and cursor mesh for blink effect
/// Regular calls to update() are required (preferably every frame) for consistent and lag-free response
///
class CursorText : public BitmapText, public Receiver {
  public:
	enum class Flag {
		// Ignore new lines
		eNoNewLine,
		// Do not control cursor blinking
		eNoAutoBlink,
		// Disable cursor and input (behaves like BitmapText)
		eInactive
	};
	using Flags = ktl::enum_flags<Flag>;

	static constexpr std::size_t npos = std::string_view::npos;

	CursorText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);

	bool set(std::string_view text) override;
	Span<Prop const> props() const override;
	bool block(State const& state) override;
	void pushSelf(Receivers& out) override;
	void popSelf() noexcept override;

	///
	/// \brief Erase character one behind cursor and decrement cursor
	///
	/// Eg: index == 0: noop; 0 < index >= 1: text.erase(index-- - 1)
	///
	void backspace();
	///
	/// \brief Erase character at cursor
	///
	/// Eg: 0 <= index < text.size: text.erase(index); index >= text.size: noop
	///
	void deleteFront();
	///
	/// \brief Insert character at cursor and increment cursor
	///
	void insert(char ch);
	///
	/// \brief Update input, and cursor (if eNoAutoBlink is not set)
	///
	void update(State const& state);
	///
	/// \brief Set inactive and unregister Receiver
	///
	void deactivate() noexcept;

	///
	/// \brief Set character to use as cursor ('|' by default)
	///
	void cursor(char ch);
	///
	/// \brief Set percentage of advanced to offset cursor back by
	///
	void offsetX(f32 nOffsetX) noexcept { m_offsetX = nOffsetX; }
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

	Flags m_flags;
	Time_s m_blinkRate = 500ms;

  private:
	struct Pen;

	bool refresh();
	void regenerate();

	graphics::Mesh m_combined;
	std::string m_text;
	std::size_t m_index = npos;
	f32 m_offsetX = 0.3f;
	time::Point m_lastBlink;
	char m_cursor = '|';
	bool m_drawCursor = true;
};
} // namespace le::input
