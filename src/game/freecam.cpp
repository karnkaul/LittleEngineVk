#include <glm/gtx/quaternion.hpp>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <engine/game/freecam.hpp>
#include <engine/window/window.hpp>

namespace le
{
using namespace input;

void FreeCam::init(Window* pWindow)
{
	m_pWindow = pWindow;
	ASSERT(m_pWindow, "Window is null!");
	m_state.speed = m_config.defaultSpeed;
	m_tMove = m_pWindow->registerInput([this](Key key, Action action, Mods::VALUE mods) {
		switch (action)
		{
		case Action::ePress:
		{
			m_state.heldKeys.insert(key);
			break;
		}
		case Action::eRelease:
		{
			if (!m_state.flags.isSet(Flag::eFixedSpeed) && key == Key::eMouseButton3)
			{
				m_state.speed = m_config.defaultSpeed;
			}
			m_state.heldKeys.erase(key);
			break;
		}
		default:
			break;
		}
		if (m_state.flags.isSet(Flag::eKeyToggle_Look) && key == m_config.lookToggle.key && action == m_config.lookToggle.action
			&& (m_config.lookToggle.mods == Mods::eNONE || m_config.lookToggle.mods & mods))
		{
			m_state.flags.flip(Flag::eLooking);
			m_state.flags.reset(Flag::eTracking);
		}
		if (key == Key::eMouseButton2 && m_state.flags.isSet(Flag::eEnabled))
		{
			bool const bLook = action == Action::ePress;
			if (!bLook || !m_state.flags.isSet(Flag::eLooking))
			{
				m_state.flags.reset(Flag::eTracking);
			}
			m_state.flags[Flag::eLooking] = bLook;
			m_pWindow->setCursorMode(bLook ? CursorMode::eDisabled : CursorMode::eDefault);
		}
	});
	m_tLook = m_pWindow->registerMouse([this](f64 x, f64 y) {
		if (m_state.flags.isSet(Flag::eEnabled) && m_state.flags.isSet(Flag::eLooking))
		{
			m_state.cursorPos.second = {(f32)x, (f32)y};
			if (!m_state.flags.isSet(Flag::eTracking))
			{
				m_state.cursorPos.first = {(f32)x, (f32)y};
				m_state.flags.set(Flag::eTracking);
			}
		}
	});
	m_tZoom = m_pWindow->registerScroll([this](f64 /*dx*/, f64 dy) { m_state.dSpeed += (f32)dy; });
	m_tFocus = m_pWindow->registerFocus([this](bool /*bFocus*/) { m_state.flags.reset(Flag::eTracking); });
	m_state.flags.set(Flag::eEnabled);
	return;
}

void FreeCam::tick(Time dt)
{
	ASSERT(m_pWindow, "Window is null!");
	if (!m_pWindow)
	{
		return;
	}
	if (!m_state.flags.isSet(Flag::eEnabled))
	{
		return;
	}
	Gamepad pad0 = m_pWindow->gamepadState(0);
	f32 const dt_s = dt.to_s();
	// Speed
	if (!m_state.flags.isSet(Flag::eFixedSpeed))
	{
		if (pad0.isPressed(Key::eGamepadButtonLeftBumper))
		{
			m_state.dSpeed -= (dt_s * 10);
		}
		else if (pad0.isPressed(Key::eGamepadButtonRightBumper))
		{
			m_state.dSpeed += (dt_s * 10);
		}
		if (m_state.dSpeed * m_state.dSpeed > 0.0f)
		{
			m_state.speed = std::clamp(m_state.speed + (m_state.dSpeed * dt_s * 100), m_config.minSpeed, m_config.maxSpeed);
			m_state.dSpeed = maths::lerp(m_state.dSpeed, 0.0f, 0.75f);
			if (m_state.dSpeed * m_state.dSpeed < 0.01f)
			{
				m_state.dSpeed = 0.0f;
			}
		}
	}

	// Elevation
	glm::vec2 const padTrigger(pad0.axis(PadAxis::eRightTrigger), pad0.axis(PadAxis::eLeftTrigger));
	f32 const elevation = Window::triggerToAxis(padTrigger.x) - Window::triggerToAxis(padTrigger.y);
	if (std::abs(elevation) > 0.01f)
	{
		m_position.y += (elevation * dt_s * m_state.speed);
	}

	// Look
	f32 dLook = m_config.padLookSens * dt_s;
	glm::vec2 const padRight(pad0.axis(PadAxis::eRightX), pad0.axis(PadAxis::eRightY));
	if (glm::length2(padRight) > m_config.padStickEpsilon)
	{
		m_state.yaw += (padRight.x * dLook);
		m_state.pitch += (-padRight.y * dLook);
	}

	dLook = m_config.mouseLookSens;
	glm::vec2 const dCursorPos = m_state.cursorPos.second - m_state.cursorPos.first;
	if (glm::length2(dCursorPos) > m_config.mouseLookEpsilon)
	{
		m_state.yaw += (dCursorPos.x * dLook);
		m_state.pitch += (-dCursorPos.y * dLook);
		m_state.cursorPos.first = m_state.cursorPos.second;
	}
	glm::quat const pitch = glm::angleAxis(glm::radians(m_state.pitch), gfx::g_nRight);
	glm::quat const yaw = glm::angleAxis(glm::radians(m_state.yaw), -gfx::g_nUp);
	m_orientation = yaw * pitch;

	// Move
	glm::vec3 dPos = {};
	glm::vec3 const nForward = glm::normalize(glm::rotate(m_orientation, -gfx::g_nFront));
	glm::vec3 const nRight = glm::normalize(glm::rotate(m_orientation, gfx::g_nRight));
	glm::vec2 const padLeft(pad0.axis(PadAxis::eLeftX), pad0.axis(PadAxis::eLeftY));

	if (glm::length2(padLeft) > m_config.padStickEpsilon)
	{
		dPos += (nRight * padLeft.x);
		dPos += (nForward * -padLeft.y);
	}

	glm::vec3 dPosKB = {};
	bool bIgnoreDPos = false;
	for (auto key : m_state.heldKeys)
	{
		if (bIgnoreDPos)
		{
			break;
		}
		switch (key)
		{
		default:
			break;

		case Key::eW:
		case Key::eUp:
		{
			dPosKB += nForward;
			break;
		}
		case Key::eD:
		case Key::eRight:
		{
			dPosKB += nRight;
			break;
		}
		case Key::eS:
		case Key::eDown:
		{
			dPosKB -= nForward;
			break;
		}
		case Key::eA:
		case Key::eLeft:
		{
			dPosKB -= nRight;
			break;
		}
		case Key::eLeftShift:
		case Key::eRightShift:
		case Key::eLeftControl:
		case Key::eRightControl:
		{
			bIgnoreDPos = true;
			break;
		}
		}
	}
	if (!bIgnoreDPos && glm::length2(dPosKB) > 0.0f)
	{
		dPos += glm::normalize(dPosKB);
	}
	if (!bIgnoreDPos && glm::length2(dPos) > 0.0f)
	{
		m_position += (dPos * dt_s * m_state.speed);
	}
	return;
}

void FreeCam::reset(bool bOrientation, bool bPosition)
{
	m_state.dSpeed = 0.0f;
	if (bOrientation)
	{
		m_state.pitch = m_state.yaw = 0.0f;
		m_orientation = gfx::g_qIdentity;
	}
	if (bPosition)
	{
		m_position = {};
	}
	m_state.heldKeys.clear();
	m_state.flags.reset({Flag::eTracking, Flag::eLooking});
	return;
}
} // namespace le
