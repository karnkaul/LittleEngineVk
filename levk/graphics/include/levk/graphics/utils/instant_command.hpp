#pragma once
#include <levk/graphics/utils/command_pool.hpp>

namespace le::graphics {
class VRAM;

class InstantCommand {
  public:
	explicit InstantCommand(not_null<CommandPool*> pool, bool block = false);
	explicit InstantCommand(not_null<VRAM*> vram, bool block = false);
	~InstantCommand();

	CommandBuffer const& cb() const noexcept { return m_cb; }

  private:
	not_null<CommandPool*> m_pool;
	CommandBuffer m_cb;
	bool m_block;
};

class BlockingCommand : public InstantCommand {
  public:
	explicit BlockingCommand(not_null<CommandPool*> pool);
	explicit BlockingCommand(not_null<VRAM*> vram);
};
} // namespace le::graphics
