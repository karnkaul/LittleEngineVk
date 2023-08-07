#include <le/audio/device.hpp>
#include <le/core/nvec3.hpp>
#include <algorithm>

namespace le::audio {
auto Device::set_transform(glm::vec3 const& position, glm::quat const& orientation) -> void {
	m_device.set_position({position.x, position.y, position.z});
	auto const look_at = orientation * -front_v;
	m_device.set_orientation(capo::Orientation{.look_at = {look_at.x, look_at.y, look_at.z}, .up = {0.0f, 1.0f, 0.0f}});
}
} // namespace le::audio
