#include <le/audio/device.hpp>
#include <le/scene/entity.hpp>
#include <le/scene/sound_controller.hpp>

namespace le {
SoundController::SoundController(Entity& entity) : Component(entity) { source = audio::Device::self().make_sound_source(); }

auto SoundController::tick(Duration /*dt*/) -> void {
	auto const position = get_entity().global_position();
	source.set_position({position.x, position.y, position.z});
}
} // namespace le
