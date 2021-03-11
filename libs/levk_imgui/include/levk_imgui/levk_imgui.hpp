#pragma once
#include <cstdint>
#include <core/erased_ref.hpp>
#include <core/mono_instance.hpp>
#include <core/ref.hpp>
#include <core/std_types.hpp>
#include <vulkan/vulkan.hpp>

#if !defined(LEVK_USE_IMGUI)
constexpr bool levk_imgui = true;
#else
constexpr bool levk_imgui = false;
#endif

namespace le {
namespace graphics {
class Device;
}
namespace window {
class DesktopInstance;
}

class DearImGui final : public TMonoInstance<DearImGui> {
  public:
	enum class State { eEnd, eBegin, eRender };

	using DesktopInstance = window::DesktopInstance;
	struct CreateInfo;

	DearImGui(graphics::Device& device);
	DearImGui(graphics::Device& device, DesktopInstance const& window, CreateInfo const& info);
	DearImGui(DearImGui&&) = default;
	DearImGui& operator=(DearImGui&&) = default;
	~DearImGui();

	bool beginFrame();
	bool render();
	bool endFrame(vk::CommandBuffer commandBuffer);

	bool demo(bool* show = nullptr) const;

	State state() const noexcept;

  private:
	bool next(State from, State to);

	Ref<graphics::Device> m_device;
	vk::DescriptorPool m_pool;
	State m_state = State::eEnd;
};

struct DearImGui::CreateInfo {
	vk::RenderPass renderPass;
	u32 descriptorCount = 1000;
	u8 imageCount = 3;
	u8 minImageCount = 2;

	explicit CreateInfo(vk::RenderPass renderPass) : renderPass(renderPass) {
	}
};

// impl

inline DearImGui::State DearImGui::state() const noexcept {
	return m_state;
}
} // namespace le
