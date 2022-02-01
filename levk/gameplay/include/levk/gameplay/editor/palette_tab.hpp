#pragma once
#include <levk/gameplay/editor/palette.hpp>
#include <levk/gameplay/editor/types.hpp>
#include <memory>
#include <vector>

namespace le::editor {
class PaletteTab : public utils::VBase {
  public:
	glm::vec2 s_minSize = {100.0f, 100.0f};

	template <typename T, typename... Args>
	T& attach(std::string id, Args&&... args);
	bool detach(std::string_view id);

	bool update(std::string_view id, glm::vec2 size, glm::vec2 pos, SceneRef scene);

  protected:
	virtual void loopItems(SceneRef scene);

	struct Entry {
		std::string id;
		std::unique_ptr<Palette> palette;
	};
	std::vector<Entry> m_items;
};

// impl

template <typename T, typename... Args>
T& PaletteTab::attach(std::string id, Args&&... args) {
	static_assert(std::is_base_of_v<Palette, T>, "T must derive from Control");
	m_items.push_back({std::move(id), std::make_unique<T>(std::forward<Args>(args)...)});
	return static_cast<T&>(*m_items.back().palette);
}
} // namespace le::editor
