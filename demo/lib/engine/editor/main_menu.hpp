#pragma once
#include <engine/editor/types.hpp>

namespace le::edi {
class MainMenu {
  public:
	MainMenu();

	void operator()(Span<MenuTree> extras) const;

  private:
	std::vector<MenuTree> m_main;
};
} // namespace le::edi
