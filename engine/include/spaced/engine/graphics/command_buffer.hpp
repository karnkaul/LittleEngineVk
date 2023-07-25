#pragma once
#include <vulkan/vulkan.hpp>

namespace spaced::graphics {
class CommandBuffer {
  public:
	CommandBuffer(CommandBuffer const&) = delete;
	CommandBuffer(CommandBuffer&&) = delete;
	auto operator=(CommandBuffer const&) -> CommandBuffer& = delete;
	auto operator=(CommandBuffer&&) -> CommandBuffer& = delete;

	CommandBuffer();
	~CommandBuffer() { submit(); }

	[[nodiscard]] auto get() const -> vk::CommandBuffer { return m_cb; }

	auto submit() -> void;

	operator vk::CommandBuffer() const { return get(); }

  private:
	vk::UniqueCommandPool m_pool{};
	vk::CommandBuffer m_cb{};
};
} // namespace spaced::graphics
