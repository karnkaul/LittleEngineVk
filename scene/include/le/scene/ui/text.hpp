#pragma once
#include <le/graphics/font/font.hpp>
#include <le/scene/ui/renderable.hpp>
#include <le/vfs/uri.hpp>

namespace le::ui {
enum class HorzAlign { eLeft, eMid, eRight };
enum class VertAlign { eTop, eMid, eBottom };

class Text : public Renderable {
  public:
	struct Align {
		HorzAlign horz{HorzAlign::eMid};
		VertAlign vert{VertAlign::eTop};
	};

	inline static Uri default_font_uri{"fonts/default.ttf"}; // NOLINT

	Text(NotNull<View*> parent_view);

	[[nodiscard]] auto get_font() const -> graphics::Font& { return *m_font; }
	auto set_font(NotNull<graphics::Font*> font) -> Text&;

	[[nodiscard]] auto get_text() const -> std::string_view { return m_text; }
	auto set_text(std::string text) -> Text&;

	[[nodiscard]] auto get_height() const -> graphics::TextHeight { return m_height; }
	auto set_height(graphics::TextHeight height) -> Text&;

	[[nodiscard]] auto get_align() const -> Align { return m_align; }
	auto set_align(Align align) -> Text&;

	[[nodiscard]] auto get_text_start() const -> glm::vec2 { return m_text_start; }

	auto tick(Duration dt) -> void override;
	auto render(std::vector<graphics::RenderObject>& out) const -> void override;

  private:
	auto set_dirty() -> Text&;
	auto refresh() -> void;

	auto render_text(std::vector<graphics::RenderObject>& out) const -> void;

	std::string m_text{};
	Ptr<graphics::Font> m_font{};
	graphics::TextHeight m_height{graphics::TextHeight::eDefault};
	Align m_align{};

	graphics::DynamicPrimitive m_text_primitive{};
	graphics::UnlitMaterial m_text_material{};
	glm::vec2 m_text_start{};
	bool m_text_dirty{false};
};
} // namespace le::ui
