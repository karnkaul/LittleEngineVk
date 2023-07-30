#pragma once
#include <spaced/core/ptr.hpp>
#include <spaced/graphics/subpass.hpp>

struct GLFWwindow;

namespace spaced::graphics {
class DearImGui : public Subpass {
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

	[[nodiscard]] auto get_load() const -> Load override;
	auto render(vk::CommandBuffer cmd) -> void final;

	vk::UniqueDescriptorPool m_pool{};
	State m_state{};
};
} // namespace spaced::graphics
