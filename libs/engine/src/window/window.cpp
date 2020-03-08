#include <memory>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/window/window.hpp"
#include "window_impl.hpp"

namespace le
{
namespace
{
WindowID g_nextWindowID = WindowID::Null;
} // namespace

Window::Service::Service()
{
	if (!WindowImpl::init())
	{
		throw std::runtime_error("Failed to initialise Window Service!");
	}
}

Window::Service::~Service()
{
	WindowImpl::deinit();
}

std::string const Window::s_tName = utils::tName<Window>();

Window::Window()
{
	m_id = ++g_nextWindowID.handle;
	m_uImpl = std::make_unique<WindowImpl>(this);
	LOG_I("[{}:{}] constructed", s_tName, m_id);
}

Window::Window(Window&&) = default;
Window& Window::operator=(Window&&) = default;
Window::~Window()
{
	m_uImpl.reset();
	LOG_I("[{}:{}] destroyed", s_tName, m_id);
}

void Window::pollEvents()
{
	WindowImpl::pollEvents();
	return;
}

WindowID Window::id() const
{
	return m_id;
}

bool Window::isOpen() const
{
	return m_uImpl ? m_uImpl->isOpen() : false;
}

bool Window::isClosing() const
{
	return m_uImpl ? m_uImpl->isClosing() : false;
}

glm::ivec2 Window::framebufferSize() const
{
	return m_uImpl ? m_uImpl->framebufferSize() : glm::ivec2(0);
}

bool Window::create(Data const& data)
{
	return m_uImpl ? m_uImpl->create(data) : false;
}

void Window::close()
{
	if (m_uImpl)
	{
		m_uImpl->close();
	}
	return;
}

void Window::destroy()
{
	if (m_uImpl)
	{
		m_uImpl->destroy();
	}
	return;
}

OnText::Token Window::registerText(OnText::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onText.subscribe(callback) : OnText::Token();
}

OnInput::Token Window::registerInput(OnInput::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onInput.subscribe(callback) : OnInput::Token();
}

OnMouse::Token Window::registerMouse(OnMouse::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onMouse.subscribe(callback) : OnMouse::Token();
}

OnMouse::Token Window::registerScroll(OnMouse::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onScroll.subscribe(callback) : OnMouse::Token();
}

OnFiledrop::Token Window::registerFiledrop(OnFiledrop::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onFiledrop.subscribe(callback) : OnFiledrop::Token();
}

OnFocus::Token Window::registerFocus(OnFocus::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onFocus.subscribe(callback) : OnFocus::Token();
}

OnWindowResize::Token Window::registerResize(OnWindowResize::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onWindowResize.subscribe(callback) : OnWindowResize::Token();
}

OnClosed::Token Window::registerClosed(OnClosed::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onClosed.subscribe(callback) : OnClosed::Token();
}

void Window::setCursorMode(CursorMode mode) const
{
	if (m_uImpl)
	{
		m_uImpl->setCursorMode(mode);
	}
	return;
}

CursorMode Window::cursorMode() const
{
	return m_uImpl ? m_uImpl->cursorMode() : CursorMode::Default;
}

glm::vec2 Window::cursorPos() const
{
	return m_uImpl ? m_uImpl->cursorPos() : glm::vec2(0.0f);
}

void Window::setCursorPos(glm::vec2 const& pos) const
{
	if (m_uImpl)
	{
		m_uImpl->setCursorPos(pos);
	}
	return;
}

std::string Window::getClipboard() const
{
	return m_uImpl ? m_uImpl->getClipboard() : std::string();
}

JoyState Window::getJoyState(s32 id)
{
	return WindowImpl::getJoyState(id);
}

GamepadState Window::getGamepadState(s32 id)
{
	return WindowImpl::getGamepadState(id);
}

f32 Window::triggerToAxis(f32 triggerValue)
{
	return WindowImpl::triggerToAxis(triggerValue);
}

size_t Window::joystickAxesCount(s32 id)
{
	return WindowImpl::joystickAxesCount(id);
}

size_t Window::joysticKButtonsCount(s32 id)
{
	return WindowImpl::joysticKButtonsCount(id);
}

std::string_view Window::toString(s32 key)
{
	return WindowImpl::toString(key);
}
} // namespace le
