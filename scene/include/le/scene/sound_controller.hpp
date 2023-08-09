#pragma once
#include <capo/sound_source.hpp>
#include <le/audio/volume.hpp>
#include <le/scene/component.hpp>

namespace le {
using capo::SoundSource;

class SoundController : public Component {
  public:
	auto setup() -> void override;
	auto tick(Duration dt) -> void override;

	[[nodiscard]] auto get_volume() const -> Volume { return Gain{source.gain()}; }
	auto set_volume(Volume volume) -> void { source.set_gain(volume.normalized()); }

	SoundSource source{};
};
} // namespace le
