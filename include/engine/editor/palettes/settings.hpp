#pragma once
#include <engine/editor/palette.hpp>

namespace le::edi {
class Settings : public Palette {
  public:
	void update() override;
};
} // namespace le::edi
