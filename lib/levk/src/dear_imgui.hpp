#pragma once
#include <levk/core/polymorphic.hpp>

namespace levk {
class IDearImGui : public Polymorphic {
  public:
	virtual void new_frame() = 0;
	virtual void end_frame() = 0;
	virtual void render(vk::CommandBuffer command_buffer) = 0;
};
} // namespace levk
