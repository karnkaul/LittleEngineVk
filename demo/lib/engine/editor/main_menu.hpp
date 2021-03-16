#pragma once
#include <engine/editor/types.hpp>

namespace le::edi {
class MainMenu {
  public:
	MainMenu();

	void operator()(Span<Menu> extras) const;

  private:
	std::vector<Menu> m_main;
};
} // namespace le::edi
