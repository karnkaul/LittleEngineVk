#pragma once
#include <engine/editor/types.hpp>

namespace le::graphics {
class ARenderer;
}

namespace le::edi {
class MainMenu {
  public:
	MainMenu();

	void operator()(MenuList const& extras, graphics::ARenderer& renderer) const;

  private:
	MenuList m_main;
};
} // namespace le::edi
