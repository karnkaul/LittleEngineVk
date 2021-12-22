#pragma once
#include <graphics/command_buffer.hpp>
#include <graphics/utils/defer.hpp>

namespace le::graphics {
class CommandPool {
  public:
	struct Cmd {
		Defer<vk::Fence> fence;
		CommandBuffer cb;

		static Cmd make(Device* device, vk::CommandBuffer cb);
	};

	static constexpr auto pool_flags_v = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;

	CommandPool(not_null<Device*> device, QType qtype = QType::eGraphics, std::size_t batch = 4);

	Cmd acquire();
	void release(Cmd&& cmd);

  private:
	std::vector<Cmd> m_cbs;
	Defer<vk::CommandPool> m_pool;
	not_null<Device*> m_device;
	QType m_qtype{};
	u32 m_batch;
};
} // namespace le::graphics
