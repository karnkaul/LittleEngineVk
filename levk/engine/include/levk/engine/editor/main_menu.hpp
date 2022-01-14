#pragma once
#include <levk/engine/editor/types.hpp>

namespace le::edi {
class MainMenu {
  public:
	MainMenu();

	void operator()(MenuList const& extras) const;

  private:
	MenuList m_main;
};
} // namespace le::edi
