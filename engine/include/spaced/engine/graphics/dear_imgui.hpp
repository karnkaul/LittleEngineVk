#pragma once
#include <spaced/engine/core/ptr.hpp>
#include <spaced/engine/graphics/render_pass.hpp>

struct GLFWwindow;

namespace spaced::graphics {
class DearImGui : public RenderPass {
  public:
	DearImGui(DearImGui&&) = delete;
	DearImGui(DearImGui const&) = delete;

	auto operator=(DearImGui&&) -> DearImGui& = delete;
	auto operator=(DearImGui const&) -> DearImGui& = delete;

	DearImGui(Ptr<GLFWwindow> window, vk::Format colour, vk::Format depth);
	~DearImGui() override;

	auto new_frame() -> void;
	auto end_frame() -> void;

  private:
	enum class State { eNewFrame, eEndFrame };

	auto render(vk::CommandBuffer cmd) -> void final;

	vk::UniqueDescriptorPool m_pool{};
	State m_state{};
};
} // namespace spaced::graphics
