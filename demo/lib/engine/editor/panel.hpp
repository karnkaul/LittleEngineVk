#pragma once
#include <memory>
#include <unordered_map>
#include <engine/editor/types.hpp>
#include <engine/editor/widget.hpp>

namespace le::edi {
class Palette {
  public:
	glm::vec2 s_minSize = {100.0f, 100.0f};

	template <typename T, typename... Args>
	T& attach(std::string id, Args&&... args);
	bool detach(std::string const& id);

	bool update(std::string_view id, glm::vec2 size, glm::vec2 pos);

  private:
	std::unordered_map<std::string, std::unique_ptr<Control>> m_items;
};

// impl

template <typename T, typename... Args>
T& Palette::attach(std::string id, Args&&... args) {
	static_assert(std::is_base_of_v<Control, T>, "T must derive from Control");
	auto const [it, res] = m_items.emplace(std::move(id), std::make_unique<T>(std::forward<Args>(args)...));
	return static_cast<T&>(*it->second);
}
} // namespace le::edi
