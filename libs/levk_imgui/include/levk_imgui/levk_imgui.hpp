#pragma once
#include <core/not_null.hpp>
#include <core/std_types.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/deferred.hpp>
#include <cstdint>

#if defined(LEVK_USE_IMGUI)
#include <imgui.h>

constexpr bool levk_imgui = true;
#else
constexpr bool levk_imgui = false;
#endif

namespace le {
namespace graphics {
class RenderContext;
class CommandBuffer;
} // namespace graphics
namespace window {
class Instance;
}

class DearImGui final {
  public:
	enum class State { eEnd, eBegin, eRender };

	using Window = window::Instance;

	DearImGui();
	DearImGui(not_null<graphics::RenderContext*> context, not_null<Window const*> window, std::size_t descriptorCount = 1000);

	bool beginFrame();
	bool endFrame();
	bool renderDrawData(graphics::CommandBuffer const& cb);

	State state() const noexcept;
	bool ready() const noexcept;

	bool m_showDemo = false;

  private:
	struct Del {
		void operator()(vk::Device, void*) const;
	};

	bool next(State from, State to);

#if defined(LEVK_USE_IMGUI)
	graphics::Device* m_device = {};
	graphics::Deferred<vk::DescriptorPool> m_pool;
	graphics::Deferred<void*, Del> m_del;
#endif
	State m_state = State::eEnd;
};

// impl

inline DearImGui::State DearImGui::state() const noexcept { return m_state; }
inline bool DearImGui::ready() const noexcept { return m_state == State::eBegin; }
} // namespace le
