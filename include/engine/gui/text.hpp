#pragma once
#include <engine/gui/tree.hpp>
#include <engine/render/bitmap_text.hpp>
#include <graphics/mesh.hpp>

namespace le {
class BitmapFont;
}

namespace le::gui {
class Text : public TreeNode {
  public:
	using Factory = graphics::TextFactory;
	using Size = Factory::Size;

	Text(not_null<TreeRoot*> root, not_null<BitmapFont const*> font) noexcept;

	Factory const& factory() const noexcept { return m_text.factory; }
	std::string_view str() const noexcept { return m_str; }
	void set(std::string str, std::optional<Factory> factory = std::nullopt);
	void set(Factory factory);

	Span<Prop const> props() const noexcept override;

	not_null<BitmapFont const*> m_font;

  private:
	void onUpdate(input::Space const& space) override;

	BitmapText m_text;
	std::string m_str;
	bool m_dirty = false;
};

// impl

inline void Text::set(std::string str, std::optional<Factory> factory) {
	m_str = std::move(str);
	if (factory) { set(std::move(*factory)); }
	m_dirty = true;
}
inline void Text::set(Factory factory) {
	m_text.factory = std::move(factory);
	m_dirty = true;
}
} // namespace le::gui
