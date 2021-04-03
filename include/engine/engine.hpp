#pragma once
#include <core/io.hpp>
#include <core/version.hpp>
#include <engine/editor/editor.hpp>
#include <engine/input/input.hpp>
#include <engine/tagged_deque.hpp>
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

	struct CreateInfo;

	static Version version() noexcept;

	Engine(Window& winInst, CreateInfo const& info);

	Input::Out poll(bool consume) noexcept;
	void pushReceiver(Input::IReceiver& context);

	bool editorActive() const noexcept;
	bool editorEngaged() const noexcept;
	void updateEditor();

	bool boot(Boot::CreateInfo const& boot);
	bool unboot() noexcept;
	bool booted() const noexcept;

	GFX& gfx();
	GFX const& gfx() const;
	Input::State const& inputState() const noexcept;
	Desktop* desktop() const noexcept;

	vk::Viewport viewport(Viewport const& view = {}, glm::vec2 depth = {0.0f, 1.0f}) const noexcept;

	Ref<Window> m_win;

  private:
	using Receiver = Ref<Input::IReceiver>;
	using TagDeque = TaggedDeque<Receiver, InputTag::type>;
	using Receivers = TaggedStore<Receiver, InputTag, TagDeque>;

	io::Service m_io;
	std::optional<GFX> m_gfx;
	Editor m_editor;
	Input m_input;
	Receivers m_receivers;
	Input::State m_inputState;
	Desktop* m_pDesktop = {};
};

struct Engine::CreateInfo {
	std::optional<io::Path> logFile = "log.txt";
	LibLogger::Verbosity verbosity = LibLogger::libVerbosity;
};

// impl

inline bool Engine::editorActive() const noexcept {
	return m_editor.active();
}
inline bool Engine::editorEngaged() const noexcept {
	return m_editor.active() && Editor::s_engaged;
}
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
