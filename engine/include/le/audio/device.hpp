#pragma once
#include <capo/capo.hpp>
#include <glm/gtc/quaternion.hpp>
#include <le/audio/music.hpp>
#include <le/audio/sound.hpp>
#include <le/core/mono_instance.hpp>
#include <le/core/not_null.hpp>
#include <array>
#include <optional>
#include <vector>

namespace le::audio {
class Device : public MonoInstance<Device> {
  public:
	[[nodiscard]] auto get_sound() const -> Sound const& { return m_sound; }
	[[nodiscard]] auto get_sound() -> Sound& { return m_sound; }

	[[nodiscard]] auto get_music() const -> Music const& { return m_music; }
	[[nodiscard]] auto get_music() -> Music& { return m_music; }

	auto set_transform(glm::vec3 const& position, glm::quat const& orientation) -> void;

  private:
	capo::Device m_device{};
	Sound m_sound{&m_device};
	Music m_music{&m_device};
};
} // namespace le::audio
