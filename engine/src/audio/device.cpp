#include <le/audio/device.hpp>
#include <le/core/nvec3.hpp>
#include <algorithm>

namespace le::audio {
namespace {
[[nodiscard]] auto is_busy(capo::SoundSource const& source) -> bool {
	switch (source.state()) {
	case capo::State::eIdle:
	case capo::State::eStopped: return false;
	default: return true;
	}
}
} // namespace

Device::Device() : m_music_source(m_device.make_stream_source()) { set_music_volume(default_music_volume_v); }

auto Device::make_sound_source() -> capo::SoundSource {
	auto const it = std::ranges::find_if(m_sound_sources, [](auto const& source) { return !is_busy(source); });
	if (it != m_sound_sources.end()) {
		auto ret = std::move(*it);
		m_sound_sources.erase(it);
		return ret;
	}
	return m_device.make_sound_source();
}

auto Device::make_music_source() const -> MusicSource { return m_device.make_stream_source(); }

auto Device::play_sound(Clip const clip, Volume const volume, glm::vec3 const& at) -> void {
	auto source = make_sound_source();
	source.set_position({at.x, at.y, at.z});
	source.set_looping(false);
	source.set_gain(volume.normalized());
	source.set_clip(clip);
	source.play();
	m_sound_sources.push_back(std::move(source));
}

auto Device::stop_sounds() -> void {
	for (auto& source : m_sound_sources) { source.stop(); }
}

auto Device::play_music() -> bool {
	if (!has_track()) { return false; }
	m_music_source.play();
	return true;
}

auto Device::stop_music() -> bool {
	if (!has_track()) { return false; }
	m_music_source.stop();
	return true;
}

auto Device::pause_music() -> bool {
	if (!has_track()) { return false; }
	m_music_source.pause();
	return true;
}

auto Device::set_transform(glm::vec3 const& position, glm::quat const& orientation) -> void {
	m_device.set_position({position.x, position.y, position.z});
	auto const look_at = orientation * -front_v;
	m_device.set_orientation(capo::Orientation{.look_at = {look_at.x, look_at.y, look_at.z}, .up = {0.0f, 1.0f, 0.0f}});
}
} // namespace le::audio
