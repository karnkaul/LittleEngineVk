#pragma once
#include <graphics/context/command_buffer.hpp>

namespace le::graphics {
class FrameDrawer {
  public:
	FrameDrawer() = default;
	virtual ~FrameDrawer() = default;

	virtual void draw3D(CommandBuffer) = 0;
	virtual void drawUI(CommandBuffer) = 0;
};
} // namespace le::graphics
