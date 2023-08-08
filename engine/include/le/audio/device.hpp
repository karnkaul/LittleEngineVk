#pragma once
#include <capo/capo.hpp>
#include <glm/gtc/quaternion.hpp>
#include <le/audio/volume.hpp>
#include <le/core/enum_array.hpp>
#include <le/core/mono_instance.hpp>
#include <le/core/not_null.hpp>
#include <array>
#include <optional>
#include <vector>

namespace le::audio {
using capo::Clip;
using capo::Pcm;
using capo::SoundSource;
using MusicSource = capo::StreamSource;

inline constexpr StaticEnumToString<capo::State, 5> state_str_v{
	"unknown", "idle", "playing", "paused", "stopped",
};

class Device : public MonoInstance<Device> {
  public:
	static constexpr Volume default_music_volume_v{80.0f};

	Device();

	[[nodiscard]] auto make_sound_source() -> SoundSource;
	[[nodiscard]] auto make_music_source() const -> MusicSource;

	auto play_sound(Clip clip, Volume volume = Volume{100.0f}, glm::vec3 const& at = {}) -> void;
	auto stop_sounds() -> void;

	auto set_track(NotNull<Pcm const*> track) -> void { m_music_source.set_stream(track->clip()); }
	[[nodiscard]] auto has_track() const -> bool { return m_music_source.has_stream(); }

	auto play_music() -> bool;
	auto pause_music() -> bool;
	auto stop_music() -> bool;
	auto set_music_volume(Volume volume) -> void { m_music_source.set_gain(volume.normalized()); }

	auto set_transform(glm::vec3 const& position, glm::quat const& orientation) -> void;

  private:
	capo::Device m_device{};
	std::vector<SoundSource> m_sound_sources{};
	MusicSource m_music_source{};
};
} // namespace le::audio
