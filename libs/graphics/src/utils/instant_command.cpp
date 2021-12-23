#include <graphics/context/vram.hpp>
#include <graphics/utils/instant_command.hpp>

namespace le::graphics {
InstantCommand::InstantCommand(not_null<CommandPool*> pool) : m_pool(pool), m_cb(m_pool->acquire()) {}
InstantCommand::InstantCommand(not_null<VRAM*> vram) : m_pool(&vram->commandPool()), m_cb(m_pool->acquire()) {}

InstantCommand::~InstantCommand() { m_pool->release(std::move(m_cb)); }
} // namespace le::graphics
