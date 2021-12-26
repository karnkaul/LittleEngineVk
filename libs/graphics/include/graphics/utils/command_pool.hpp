#pragma once
#include <graphics/command_buffer.hpp>
#include <graphics/utils/defer.hpp>

namespace le::graphics {
class FencePool {
  public:
	FencePool(not_null<Device*> device, std::size_t count = 0);

	vk::Fence next();

  private:
	Defer<vk::Fence> makeFence() const { return Defer<vk::Fence>::make(m_device->makeFence(true), m_device); }

	std::vector<Defer<vk::Fence>> m_free;
	std::vector<Defer<vk::Fence>> m_busy;
	not_null<Device*> m_device;
};

class CommandPool {
  public:
	static constexpr auto pool_flags_v = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;

	CommandPool(not_null<Device*> device, QType qtype = QType::eGraphics, std::size_t batch = 4);

	CommandBuffer acquire();
	vk::Result release(CommandBuffer&& cmd, bool block);

  private:
	struct Cmd {
		vk::CommandBuffer cb;
		vk::Fence fence;
	};

	FencePool m_fencePool;
	std::vector<Cmd> m_cbs;
	Defer<vk::CommandPool> m_pool;
	not_null<Device*> m_device;
	QType m_qtype{};
	u32 m_batch;
};
} // namespace le::graphics
