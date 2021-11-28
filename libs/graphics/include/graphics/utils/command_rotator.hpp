#pragma once
#include <graphics/command_buffer.hpp>
#include <graphics/qflags.hpp>
#include <graphics/utils/deferred.hpp>
#include <graphics/utils/ring_buffer.hpp>
#include <ktl/future.hpp>

namespace le::graphics {
class CommandRotator {
  public:
	class Pool {
	  public:
		struct Cmd {
			Deferred<vk::Fence> fence;
			vk::CommandBuffer cb;

			static Cmd make(Device* device, vk::CommandBuffer cb);
		};

		static constexpr auto pool_flags_v = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;

		Pool(not_null<Device*> device, QType qtype = QType::eGraphics, std::size_t batch = 4);

		Cmd acquire();
		void release(Cmd&& cmd);

	  private:
		std::vector<Cmd> m_cbs;
		Deferred<vk::CommandPool> m_pool;
		not_null<Device*> m_device;
		u32 m_batch;
	};

	static constexpr auto pool_flags_v = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;

	CommandRotator(not_null<Device*> device, QType qtype = QType::eGraphics);

	void immediate(ktl::move_only_function<void(CommandBuffer const&)>&& func);

	CommandBuffer const& get() const noexcept { return m_current.cb; }
	vk::Fence fence() const noexcept { return m_current.fence; }
	ktl::future<void> future() const { return m_current.promise.get_future(); }

	void submit();

  private:
	struct Cmd {
		CommandBuffer cb;
		Deferred<vk::Fence> fence;
		mutable ktl::promise<void> promise;
	};

	void releaseDone();
	void updateCurrent();

	Pool m_pool;
	Cmd m_current;
	std::vector<Cmd> m_submitted;
	QType m_qtype;
	not_null<Device*> m_device;
};
} // namespace le::graphics
