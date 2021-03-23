#include <engine/engine.hpp>
#include <window/desktop_instance.hpp>

namespace le {
Engine::GFX::GFX(Window const& winst, Boot::CreateInfo const& bci) : boot(bci, makeSurface(winst), winst.framebufferSize()), context(boot.swapchain) {
	if (winst.isDesktop()) {
		DearImGui::CreateInfo dici(boot.swapchain.renderPass());
		dici.texFormat = context.textureFormat();
		imgui = DearImGui(boot.device, static_cast<window::DesktopInstance const&>(winst), dici);
	}
}

Engine::Boot::MakeSurface Engine::GFX::makeSurface(Window const& winst) {
	return [&winst](vk::Instance vkinst) {
		vk::SurfaceKHR ret;
		winst.vkCreateSurface(vkinst, ret);
		return ret;
	};
}

Engine::Engine(Window& winInst) : m_win(winInst), m_pDesktop(winInst.isDesktop() ? static_cast<window::DesktopInstance*>(&winInst) : nullptr) {
}

Input::Out Engine::poll(bool consume) noexcept {
	auto ret = m_input.update(m_win.get().pollEvents(), m_editor.view(), consume, m_pDesktop);
	m_inputState = ret.state;
	for (Input::IReceiver& context : m_receivers) {
		if (context.block(ret.state)) {
			break;
		}
	}
	return ret;
}

void Engine::pushReceiver(Input::IReceiver& context) {
	context.m_inputToken = m_receivers.push<true>(context);
}

void Engine::updateEditor() {
	if constexpr (levk_imgui) {
		if (m_gfx) {
			m_gfx->imgui.beginFrame();
		}
		ENSURE(m_pDesktop, "Invariant violated");
		m_editor.update(*m_pDesktop, m_inputState);
	}
}

bool Engine::boot(Boot::CreateInfo const& boot) {
	if (!m_gfx) {
		m_gfx.emplace(m_win, boot);
		return true;
	}
	return false;
}

bool Engine::unboot() noexcept {
	if (m_gfx) {
		m_gfx.reset();
		return true;
	}
	return false;
}

vk::Viewport Engine::viewport(Viewport const& view, glm::vec2 depth) const noexcept {
	if (!m_gfx) {
		return {};
	}
	Viewport const vp = view * m_editor.view();
	return m_gfx->context.viewport(m_win.get().framebufferSize(), depth, vp.rect(), vp.topLeft.offset);
}
} // namespace le
