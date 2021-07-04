#pragma once
#include <graphics/render/command_buffer.hpp>
#include <graphics/utils/deferred.hpp>

namespace le::graphics {
class CommandPool {
  public:
	CommandPool(not_null<Device*> device, vk::CommandPoolCreateFlags flags, QType qtype = QType::eGraphics);

	void update();
	template <typename Fn>
	void submit(Fn fn, vk::Semaphore wait = {}, vk::PipelineStageFlags stages = {}, vk::Semaphore signal = {}) const;

  private:
	struct Cmd {
		CommandBuffer cb;
		Deferred<vk::Fence> fence;
	};

	Cmd newCmd() const;
	Cmd beginCmd() const;
	void submit(Cmd cmd, vk::Semaphore wait, vk::PipelineStageFlags stages, vk::Semaphore signal) const;

	Deferred<vk::CommandPool> m_pool;
	mutable std::vector<Cmd> m_free;
	mutable std::vector<Cmd> m_busy;
	QType m_qtype;
	not_null<Device*> m_device;
};

// impl

template <typename Fn>
void CommandPool::submit(Fn fn, vk::Semaphore wait, vk::PipelineStageFlags stages, vk::Semaphore signal) const {
	auto cmd = beginCmd();
	fn(cmd.cb);
	submit(std::move(cmd), wait, stages, signal);
}
} // namespace le::graphics
