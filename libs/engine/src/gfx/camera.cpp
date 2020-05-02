#include <glm/gtx/quaternion.hpp>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/maths.hpp"
#include "engine/gfx/geometry.hpp"
// #include "engine/gfx/utils.hpp"
// #include "engine/window/input.hpp"
#include "engine/gfx/camera.hpp"
#include "engine/window/window.hpp"

namespace le::gfx
{
Camera::Camera() = default;
Camera::Camera(Camera&&) = default;
Camera& Camera::operator=(Camera&&) = default;
Camera::~Camera() = default;

void Camera::tick(Time) {}

bool s_bTEST = false;
glm::mat4 Camera::view() const
{
#if defined(LEVK_DEBUG)
	if (s_bTEST)
	{
		glm::vec3 const nFront = -glm::normalize(glm::rotate(m_orientation, g_nFront));
		glm::vec3 const nUp = glm::normalize(glm::rotate(m_orientation, g_nUp));
		return glm::lookAt(m_position, m_position + nFront, nUp);
	}
	else
#endif
	{
		return glm::toMat4(glm::conjugate(m_orientation)) * glm::translate(glm::mat4(1.0f), -m_position);
	}
}

glm::mat4 Camera::perspectiveProj(f32 aspect, f32 near, f32 far) const
{
	return glm::perspective(glm::radians(m_fov), m_aspectRatio > 0.0f ? m_aspectRatio : aspect, near, far);
}

glm::mat4 Camera::orthographicProj(f32 aspect, f32 zoom, f32 near, f32 far) const
{
	ASSERT(zoom > 0.0f, "Invalid zoom!");
	f32 ar = m_aspectRatio > 0.0f ? m_aspectRatio : aspect;
	f32 w = ar > 1.0f ? 1.0f : 1.0f * ar;
	f32 h = ar > 1.0f ? 1.0f / ar : 1.0f;
	return glm::ortho(-w / zoom, w / zoom, -h / zoom, h / zoom, near, far);
}

glm::mat4 Camera::uiProj(glm::vec3 const& uiSpace) const
{
	f32 const w = uiSpace.x * 0.5f;
	f32 const h = uiSpace.y * 0.5f;
	return glm::ortho(-w, w, -h, h, -uiSpace.z, uiSpace.z);
}

FreeCam::FreeCam(Window* pWindow) : m_pWindow(pWindow)
{
	ASSERT(m_pWindow, "Window is null!");
	m_state.speed = m_config.defaultSpeed;
	m_tMove = m_pWindow->registerInput([this](Key key, Action action, Mods /*mods*/) {
		if (m_state.flags.isSet(Flag::eEnabled))
		{
			switch (action)
			{
			case Action::ePress:
			{
				m_state.heldKeys.emplace(key);
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
			if (key == Key::eMouseButton2)
			{
				bool bLook = action == Action::ePress;
				if (m_state.flags.isSet(Flag::eLooking) ^ bLook)
				{
					m_state.flags.reset(Flag::eTracking);
				}
				m_state.flags.bits[((size_t)Flag::eLooking)] = bLook;
				m_pWindow->setCursorMode(bLook ? CursorMode::eDisabled : CursorMode::eDefault);
			}
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

FreeCam::FreeCam(FreeCam&&) = default;
FreeCam& FreeCam::operator=(FreeCam&&) = default;
FreeCam::~FreeCam() = default;

void FreeCam::tick(Time dt)
{
	if (!m_state.flags.isSet(Flag::eEnabled))
	{
		return;
	}
	GamepadState pad0 = m_pWindow->getGamepadState(0);

	// Speed
	if (!m_state.flags.isSet(Flag::eFixedSpeed))
	{
		if (pad0.isPressed(Key::eGamepadButtonLeftBumper))
		{
			m_state.dSpeed -= (dt.to_s() * 10);
		}
		else if (pad0.isPressed(Key::eGamepadButtonRightBumper))
		{
			m_state.dSpeed += (dt.to_s() * 10);
		}
		if (m_state.dSpeed * m_state.dSpeed > 0.0f)
		{
			m_state.speed = maths::clamp(m_state.speed + (m_state.dSpeed * dt.to_s() * 100), m_config.minSpeed, m_config.maxSpeed);
			m_state.dSpeed = maths::lerp(m_state.dSpeed, 0.0f, 0.75f);
			if (m_state.dSpeed * m_state.dSpeed < 0.01f)
			{
				m_state.dSpeed = 0.0f;
			}
		}
	}

	// Elevation
	f32 elevation =
		m_pWindow->triggerToAxis(pad0.getAxis(PadAxis::eRightTrigger)) - m_pWindow->triggerToAxis(pad0.getAxis(PadAxis::eLeftTrigger));
	if (elevation * elevation > 0.01f)
	{
		m_position.y += (elevation * dt.to_s() * m_state.speed);
	}

	// Look
	f32 dLook = m_config.padLookSens * dt.to_s();
	glm::vec2 const padRight(pad0.getAxis(PadAxis::eRightX), -pad0.getAxis(PadAxis::eRightY));
	if (glm::length2(padRight) > m_config.padStickEpsilon)
	{
		m_state.pitch += (padRight.y * dLook);
		m_state.yaw += (padRight.x * dLook);
	}

	dLook = m_config.mouseLookSens;
	glm::vec2 dCursorPos = m_state.cursorPos.second - m_state.cursorPos.first;
	if (glm::length2(dCursorPos) > m_config.mouseLookEpsilon)
	{
		m_state.yaw += (dCursorPos.x * dLook);
		m_state.pitch += (-dCursorPos.y * dLook);
		m_state.cursorPos.first = m_state.cursorPos.second;
	}
	glm::quat pitch = glm::angleAxis(glm::radians(m_state.pitch), g_nRight);
	glm::quat yaw = glm::angleAxis(glm::radians(m_state.yaw), -g_nUp);
	m_orientation = yaw * pitch;

	// Move
	glm::vec3 dPos = glm::vec3(0.0f);
	glm::vec3 const nForward = glm::normalize(glm::rotate(m_orientation, -g_nFront));
	glm::vec3 const nRight = glm::normalize(glm::rotate(m_orientation, g_nRight));
	glm::vec2 const padLeft(pad0.getAxis(PadAxis::eLeftX), -pad0.getAxis(PadAxis::eLeftY));

	if (glm::length2(padLeft) > m_config.padStickEpsilon)
	{
		dPos += (nForward * padLeft.y);
		dPos += (nRight * padLeft.x);
	}

	for (auto key : m_state.heldKeys)
	{
		switch (key)
		{
		default:
			break;

		case Key::eW:
		case Key::eUp:
		{
			dPos += nForward;
			break;
		}

		case Key::eD:
		case Key::eRight:
		{
			dPos += nRight;
			break;
		}

		case Key::eS:
		case Key::eDown:
		{
			dPos -= nForward;
			break;
		}

		case Key::eA:
		case Key::eLeft:
		{
			dPos -= nRight;
			break;
		}
		}
	}
	if (glm::length2(dPos) > 0.0f)
	{
		dPos = glm::normalize(dPos);
		m_position += (dPos * dt.to_s() * m_state.speed);
	}
	return;
}
} // namespace le::gfx
