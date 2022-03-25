#pragma once
#include <ktl/enumerate.hpp>
#include <levk/gameplay/gui/shape.hpp>
#include <levk/gameplay/gui/text.hpp>
#include <levk/gameplay/gui/widgets/button.hpp>
#include <levk/gameplay/gui/widgets/flexbox.hpp>

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
		u32 textHeight = 40U;
		std::optional<f32> zIndex;
		Hash fontURI = defaultFontURI;
		Hash style;
	};

	template <typename T = Button>
		requires(std::is_base_of_v<Button, T>)
	struct CreateInfo : CreateInfoBase {};

	struct Select {
		std::string_view text;
		std::size_t index{};
	};

	using OnSelect = ktl::delegate<Select>;

	template <typename T = Button, typename... Args>
	Dropdown(not_null<TreeRoot*> root, CreateInfo<T> info, Args&&... args) : Button(root, info.fontURI, info.style), m_options(std::move(info.options)) {
		if (!m_options.empty()) {
			ENSURE(info.selected < m_options.size(), "Invalid index");
			init(std::move(info));
			for (auto [entry, index] : ktl::enumerate(m_options)) {
				bool const pad = itemPad(entry, index);
				add(m_flexbox->add<T>(m_rect.size, pad, m_text->m_fontURI, std::forward<Args>(args)...), entry, index);
			}
			refresh();
		}
	}

	OnSelect::signal onSelect() { return m_onSelect.make_signal(); }
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
	OnClick::signal m_onClickTk;
	std::vector<std::string> m_options;
	std::vector<OnClick::signal> m_entrySignals;
	std::size_t m_selected{};
	Flexbox* m_flexbox{};
	CreateInfoBase::TextColours m_textColours;
};
} // namespace le::gui
