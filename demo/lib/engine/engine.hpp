#pragma once
#include <engine/editor/editor.hpp>
#include <engine/input/input.hpp>
#include <graphics/context/bootstrap.hpp>
#include <graphics/render_context.hpp>
#include <levk_imgui/levk_imgui.hpp>

namespace le {
namespace window {
class IInstance;
class DesktopInstance;
} // namespace window

class Engine {
  public:
	using Window = window::IInstance;
	using Desktop = window::DesktopInstance;
	using Boot = graphics::Bootstrap;
	using Context = graphics::RenderContext;

	struct GFX {
		Boot boot;
		Context context;
		DearImGui imgui;

		GFX(Window const& winst, Boot::CreateInfo const& bci);

	  private:
		static Boot::MakeSurface makeSurface(Window const& winst);
	};

	Engine(Window& winInst);

	Input::Out poll(bool consume) noexcept;
	void pushContext(Input::IReceiver& context);
	void updateEditor();

	bool boot(Boot::CreateInfo const& boot);
	bool unboot() noexcept;

	bool booted() const noexcept;
	GFX& gfx();
	GFX const& gfx() const;
	Input::State const& inputState() const noexcept;

	vk::Viewport viewport(Viewport const& view = {}, glm::vec2 depth = {0.0f, 1.0f}) const noexcept;
	Ref<Window> m_win;

  private:
	using Receivers = TTokenGen<Ref<Input::IReceiver>, TGSpec_deque>;

	std::optional<GFX> m_gfx;
	Editor m_editor;
	Input m_input;
	Receivers m_receivers;
	Input::State m_inputState;
	Desktop* m_pDesktop = {};
};

// impl

inline bool Engine::booted() const noexcept {
	return m_gfx.has_value();
}
inline Engine::GFX& Engine::gfx() {
	ENSURE(m_gfx.has_value(), "Not booted");
	return *m_gfx;
}
inline Engine::GFX const& Engine::gfx() const {
	ENSURE(m_gfx.has_value(), "Not booted");
	return *m_gfx;
}
inline Input::State const& Engine::inputState() const noexcept {
	return m_inputState;
}
} // namespace le
