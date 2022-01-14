#include <core/log_channel.hpp>
#include <core/utils/expect.hpp>
#include <levk/graphics/context/vram.hpp>
#include <levk/graphics/utils/instant_command.hpp>

namespace le::graphics {
InstantCommand::InstantCommand(not_null<CommandPool*> pool, bool block) : m_pool(pool), m_cb(m_pool->acquire()), m_block(block) {}
InstantCommand::InstantCommand(not_null<VRAM*> vram, bool block) : m_pool(&vram->commandPool()), m_cb(m_pool->acquire()), m_block(block) {}

InstantCommand::~InstantCommand() {
	auto const res = m_pool->release(std::move(m_cb), m_block);
	EXPECT(res == vk::Result::eSuccess);
	if (res != vk::Result::eSuccess) { logW(LC_LibUser, "[Graphics] Command submission failure!"); }
}

BlockingCommand::BlockingCommand(not_null<CommandPool*> pool) : InstantCommand(pool, true) {}
BlockingCommand::BlockingCommand(not_null<VRAM*> vram) : InstantCommand(vram, true) {}
} // namespace le::graphics
