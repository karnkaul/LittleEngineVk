#pragma once
#include <core/utils/enumerate.hpp>
#include <engine/gui/shape.hpp>
#include <engine/gui/text.hpp>
#include <engine/gui/widgets/button.hpp>
#include <engine/gui/widgets/flexbox.hpp>

namespace le::gui {
class Dropdown : public Button {
  public:
	struct CreateInfoBase {
		Flexbox::CreateInfo flexbox;
		glm::vec2 size = {200.0f, 50.0f};
		std::vector<std::string> options;
		std::size_t selected = 0;
		struct TextColours {
			graphics::RGBA collapsed = colours::black;
			graphics::RGBA expanded = {Colour(0x88888888), graphics::RGBA::Type::eAbsolute};
		} textColours;
		struct CoverColours {
			graphics::RGBA cover = {Colour(0x88888888), graphics::RGBA::Type::eAbsolute};
			graphics::RGBA arrow = colours::white;
		} coverColours;
		Text::Size textSize = 40U;
		Hash style;
	};

	template <typename T = Button>
		requires(std::is_base_of_v<Button, T>)
	struct CreateInfo : CreateInfoBase {};

	struct Select {
		std::string_view text;
		std::size_t index{};
	};

	using OnSelect = Delegate<Dropdown&, Select>;

	template <typename T = Button, typename... Args>
	Dropdown(not_null<TreeRoot*> root, not_null<BitmapFont const*> font, CreateInfo<T> info, Args&&... args) noexcept
		: Button(root, font, info.style), m_options(std::move(info.options)) {
		if (!m_options.empty()) {
			ensure(info.selected < m_options.size(), "Invalid index");
			init(std::move(info));
			for (auto [entry, index] : utils::enumerate(m_options)) {
				add(m_flexbox->add<T>(m_rect.size, itemPad(entry, index), m_text->m_font, std::forward<Args>(args)...), entry, index);
			}
			refresh();
		}
	}

	OnSelect::Tk onSelect(OnSelect::Callback const& cb) { return m_onSelect.subscribe(cb); }
	bool expanded() const noexcept { return m_flexbox->m_active; }
	Select selected() const noexcept { return {m_options[m_selected], m_selected}; }

	Quad* m_cover{};
	Shape* m_arrow{};

  private:
	bool itemPad(std::string& out_org, std::size_t index) const noexcept;
	void init(CreateInfoBase info);
	void add(Button& item, std::string_view text, std::size_t index);
	void expand();
	void collapse();
	void select(std::size_t index);

	OnSelect m_onSelect;
	OnClick::Tk m_onClickTk;
	std::vector<std::string> m_options;
	std::vector<OnClick::Tk> m_entryTokens;
	std::size_t m_selected{};
	Flexbox* m_flexbox{};
	CreateInfoBase::TextColours m_textColours;
};
} // namespace le::gui
