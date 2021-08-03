#pragma once
#include <core/utils/enumerate.hpp>
#include <engine/gui/flexbox.hpp>

namespace le::gui {
class Dropdown : public Widget {
  public:
	struct CreateInfoBase {
		Flexbox::CreateInfo flexbox;
		Style<Material> quadStyle;
		Text::Factory textFactory;
		glm::vec2 size = {200.0f, 50.0f};
		std::vector<std::string> options;
		std::size_t selected = 0;
		struct TextColours {
			Colour collapsed = colours::black;
			Colour expanded = Colour(0x88888888);
		} textColours;
	};

	template <typename T = Widget>
		requires(std::is_base_of_v<Widget, T>)
	struct CreateInfo : CreateInfoBase {};

	struct Select {
		std::string_view text;
		std::size_t index{};
	};

	using OnSelect = Delegate<Dropdown&, Select>;

	template <typename T = Widget, typename... Args>
	Dropdown(not_null<TreeRoot*> root, not_null<BitmapFont const*> font, CreateInfo<T> info, Args&&... args) noexcept
		: Widget(root, font), m_textFactory(std::move(info.textFactory)), m_options(std::move(info.options)) {
		if (!m_options.empty()) {
			ensure(info.selected < m_options.size(), "Invalid index");
			init(std::move(info));
			for (auto [entry, index] : utils::enumerate(m_options)) {
				f32 const offset = itemOffset(entry, index);
				add(m_flexbox->add<T>(m_rect.size, offset, std::forward<Args>(args)...), entry, index);
			}
			refresh();
		}
	}

	OnSelect::Tk onSelect(OnSelect::Callback const& cb) { return m_onSelect.subscribe(cb); }
	bool expanded() const noexcept { return m_flexbox->m_active; }
	Select selected() const noexcept { return {m_options[m_selected], m_selected}; }

	Text::Factory m_textFactory;
	Quad* m_cover{};

  private:
	f32 itemOffset(std::string& out_org, std::size_t index) const noexcept;
	void init(CreateInfoBase info);
	void add(Widget& item, std::string_view text, std::size_t index);
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