#pragma once
#include <capo/device.hpp>
#include <glm/vec3.hpp>
#include <le/core/not_null.hpp>
#include <memory>
#include <vector>

namespace le::audio {
using capo::Clip;
using capo::SoundSource;

class Sound {
  public:
	explicit Sound(NotNull<capo::Device*> device) : m_device(device) {}

	auto make_sound_source() -> std::shared_ptr<SoundSource>;

	auto play_once(Clip clip, float gain = 1.0f, glm::vec3 const& at = {}) -> void;

  private:
	NotNull<capo::Device const*> m_device;
	std::vector<std::shared_ptr<SoundSource>> m_sources{};
};
} // namespace le::audio
