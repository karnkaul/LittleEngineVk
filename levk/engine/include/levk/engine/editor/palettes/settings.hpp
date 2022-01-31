#pragma once
#include <levk/engine/editor/palette.hpp>

namespace le::editor {
class Settings : public Palette {
  public:
	void update() override;
};
} // namespace le::editor
