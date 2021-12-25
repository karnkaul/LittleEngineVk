#include <graphics/context/vram.hpp>
#include <graphics/utils/instant_command.hpp>

namespace le::graphics {
InstantCommand::InstantCommand(not_null<CommandPool*> pool, bool block) : m_pool(pool), m_cb(m_pool->acquire()), m_block(block) {}
InstantCommand::InstantCommand(not_null<VRAM*> vram, bool block) : m_pool(&vram->commandPool()), m_cb(m_pool->acquire()), m_block(block) {}

InstantCommand::~InstantCommand() { m_pool->release(std::move(m_cb), m_block); }

BlockingCommand::BlockingCommand(not_null<CommandPool*> pool) : InstantCommand(pool, true) {}
BlockingCommand::BlockingCommand(not_null<VRAM*> vram) : InstantCommand(vram, true) {}
} // namespace le::graphics
