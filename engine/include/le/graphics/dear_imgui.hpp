#pragma once
#include <le/core/ptr.hpp>
#include <vulkan/vulkan.hpp>

struct GLFWwindow;

namespace le::graphics {
class DearImGui {
  public:
	DearImGui(DearImGui&&) = delete;
	DearImGui(DearImGui const&) = delete;

	auto operator=(DearImGui&&) -> DearImGui& = delete;
	auto operator=(DearImGui const&) -> DearImGui& = delete;

	DearImGui(Ptr<GLFWwindow> window, vk::Format colour);
	~DearImGui();

	auto new_frame() -> void;
	auto end_frame() -> void;
	auto render(vk::CommandBuffer cmd) -> void;

  private:
	enum class State { eNewFrame, eEndFrame };

	vk::UniqueDescriptorPool m_pool{};
	State m_state{};
};
} // namespace le::graphics
