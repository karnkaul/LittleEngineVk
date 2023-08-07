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

auto Sound::make_sound_source() -> std::shared_ptr<capo::SoundSource> {
	auto const it = std::ranges::find_if(m_sources, [](auto const& ptr) { return ptr.use_count() == 1 && !is_busy(*ptr); });
	if (it != m_sources.end()) { return *it; }
	m_sources.push_back(std::make_shared<SoundSource>(m_device->make_sound_source()));
	return m_sources.back();
}

auto Sound::play_once(Clip clip, float gain, glm::vec3 const& at) -> void {
	auto source = make_sound_source();
	source->set_position({at.x, at.y, at.z});
	source->set_looping(false);
	source->set_gain(gain);
	source->set_clip(clip);
	source->play();
}
} // namespace le::audio