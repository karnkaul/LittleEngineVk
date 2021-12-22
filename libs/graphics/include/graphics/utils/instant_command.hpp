#pragma once
#include <graphics/utils/command_pool.hpp>

namespace le::graphics {
class VRAM;

class InstantCommand {
  public:
	explicit InstantCommand(not_null<CommandPool*> pool);
	explicit InstantCommand(not_null<VRAM*> vram);
	~InstantCommand();

	CommandBuffer const& cb() const noexcept { return m_cmd.cb; }

  private:
	not_null<CommandPool*> m_pool;
	CommandPool::Cmd m_cmd;
};
} // namespace le::graphics
