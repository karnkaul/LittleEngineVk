#pragma once
#include <cstdint>
#include <core/erased_ref.hpp>
#include <core/mono_instance.hpp>
#include <core/ref.hpp>
#include <core/std_types.hpp>
#include <graphics/texture.hpp>

#if defined(LEVK_USE_IMGUI)
#include <imgui.h>

#define IMGUI(statemt)                                                                                                                                         \
	do {                                                                                                                                                       \
		if (auto in = DearImGui::inst(); in && in->ready()) {                                                                                                  \
			statemt;                                                                                                                                           \
		}                                                                                                                                                      \
	} while (0)

constexpr bool levk_imgui = true;
#else
constexpr bool levk_imgui = false;
#define IMGUI(x)
#endif

namespace le {
namespace graphics {
class Device;
class CommandBuffer;
} // namespace graphics
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
	bool endFrame(graphics::CommandBuffer const& cb);

	State state() const noexcept;
	bool ready() const noexcept;

	bool m_showDemo = false;

  private:
	bool next(State from, State to);

	Ref<graphics::Device> m_device;
	vk::DescriptorPool m_pool;
	State m_state = State::eEnd;
};

struct DearImGui::CreateInfo {
	vk::RenderPass renderPass;
	vk::Format texFormat = graphics::Texture::srgbFormat;
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
inline bool DearImGui::ready() const noexcept {
	return m_state == State::eBegin;
}
} // namespace le
