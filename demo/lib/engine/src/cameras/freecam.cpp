#include <core/maths.hpp>
#include <engine/cameras/freecam.hpp>
#include <window/desktop_instance.hpp>

namespace le {
void FreeCam::tick(Input::State const& state, Time_s dt, Desktop* desktop) {
	glm::vec3 const forward = glm::normalize(glm::rotate(orientation, -graphics::front));
	glm::vec3 const right = glm::normalize(glm::rotate(orientation, graphics::right));
	f32 const dx = m_controls.mov_x(state);
	f32 const dz = m_controls.mov_z(state);
	if (!maths::equals(dx, 0.0f, 0.01f) || !maths::equals(dz, 0.0f, 0.01f)) {
		glm::vec3 const dxz = glm::normalize(glm::vec3(dx, 0.0f, dz));
		glm::vec3 const dpos = 3.0f * m_params.xz_speed * (dxz.x * right + dxz.z * forward);
		position += (dpos * dt.count());
	}
	bool const look = m_controls.look(state);
	if (desktop) {
		desktop->cursorMode(look ? window::CursorMode::eDisabled : window::CursorMode::eDefault);
	}
	if (look) {
		if (!m_cursor.prev) {
			m_cursor.prev = state.cursor.position;
		}
		glm::vec2 const dlook = 3.0f * m_params.look_sens * (state.cursor.position - *m_cursor.prev) * dt.count();
		if (glm::length2(dlook) > 0.01f) {
			auto const pitch = glm::angleAxis(glm::radians(dlook.y), -graphics::right);
			auto const yaw = glm::angleAxis(glm::radians(dlook.x), -graphics::up);
			orientation = yaw * orientation * pitch;
		}
		m_cursor.prev = state.cursor.position;
	} else {
		m_cursor.prev.reset();
	}
}
} // namespace le
