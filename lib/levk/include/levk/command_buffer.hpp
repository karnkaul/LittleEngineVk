#pragma once
#include <levk/core/not_null.hpp>
#include <levk/render_device.hpp>

namespace levk {
/// \brief Vulkan Command Buffer for ad-hoc commands.
class CommandBuffer {
  public:
	CommandBuffer(IRenderDevice const& render_device);

	[[nodiscard]] auto get() const -> vk::CommandBuffer { return m_cb; }

	/// \brief Submit recorded commands and wait for them to complete on the GPU.
	void submit(IRenderDevice& render_device);

	operator vk::CommandBuffer() const { return get(); }

  private:
	vk::UniqueCommandPool m_pool{};
	vk::CommandBuffer m_cb{};
};
} // namespace levk
