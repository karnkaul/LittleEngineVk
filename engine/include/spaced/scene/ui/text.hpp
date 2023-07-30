#pragma once
#include <spaced/graphics/font/font.hpp>
#include <spaced/scene/ui/primitive_renderer.hpp>
#include <spaced/vfs/uri.hpp>

namespace spaced::ui {
class Text : public View {
  public:
	enum class Align : std::uint8_t { eLeft, eMid, eRight };

	// NOLINTNEXTLINE
	inline static Uri default_font{"fonts/default.ttf"};

	Text();

	[[nodiscard]] auto get_font() const -> graphics::Font& { return *m_font; }
	auto set_font(NotNull<graphics::Font*> font) -> Text&;

	[[nodiscard]] auto get_text() const -> std::string_view { return m_text; }
	auto set_text(std::string text) -> Text&;

	[[nodiscard]] auto get_height() const -> graphics::TextHeight { return m_height; }
	auto set_height(graphics::TextHeight height) -> Text&;

	[[nodiscard]] auto get_align() const -> Align { return m_align; }
	auto set_align(Align align) -> Text&;

	auto render_tree(Rect2D<> const& parent_frame, std::vector<graphics::RenderObject>& out) const -> void override;

  private:
	auto set_dirty() -> Text&;
	auto refresh() const -> void;

	std::string m_text{};
	Ptr<graphics::Font> m_font{};
	graphics::TextHeight m_height{graphics::TextHeight::eDefault};
	Align m_align{Align::eMid};

	mutable graphics::DynamicPrimitive m_primitive{};
	mutable graphics::UnlitMaterial m_material{};
	mutable bool m_dirty{false};
};
} // namespace spaced::ui
