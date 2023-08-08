#include <le/audio/sound.hpp>
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

auto Sound::make_sound_source() -> capo::SoundSource {
	auto const it = std::ranges::find_if(m_sources, [](auto const& source) { return !is_busy(source); });
	if (it != m_sources.end()) {
		auto ret = std::move(*it);
		m_sources.erase(it);
		return ret;
	}
	return m_device->make_sound_source();
}

auto Sound::play_once(Clip const clip, Volume const volume, glm::vec3 const& at) -> void {
	auto source = make_sound_source();
	source.set_position({at.x, at.y, at.z});
	source.set_looping(false);
	source.set_gain(volume.normalized());
	source.set_clip(clip);
	source.play();
	m_sources.push_back(std::move(source));
}

auto Sound::stop_all() -> void {
	for (auto& source : m_sources) { source.stop(); }
}
} // namespace le::audio
