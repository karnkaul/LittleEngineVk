#include <le/audio/music.hpp>
#include <le/graphics/buffering.hpp>

namespace le::audio {
Music::Music(NotNull<capo::Device*> device) {
	graphics::fill_buffered(m_sources, [device] { return device->make_stream_source(); });
}

auto Music::play(NotNull<Pcm const*> pcm, Duration cross_fade) -> void {
	if (cross_fade <= 0s) {
		auto& source = m_sources.at(m_current_stream);
		source.set_stream(pcm->clip());
		source.set_gain(volume * 0.01f);
		source.play();
		return;
	}
	stop(cross_fade);
	auto& stream = m_sources.at(m_current_stream);
	stream.set_gain(0.0f);
	stream.set_stream(pcm->clip());
	stream.play();
}

auto Music::stop(Duration cross_fade) -> void {
	auto& stream = m_sources.at(m_current_stream);
	m_current_stream = (m_current_stream + 1) % m_sources.size();
	if (cross_fade <= 0s) {
		stream.stop();
		return;
	}
	m_cross_fade = CrossFade{.total = cross_fade};
}

auto Music::tick(Duration dt) -> void {
	if (!m_cross_fade) { return; }

	m_cross_fade->elapsed += dt;
	auto const ratio = std::clamp(m_cross_fade->elapsed / m_cross_fade->total, 0.0f, 1.0f);
	auto& active = m_sources.at(m_current_stream);
	active.set_gain(ratio * volume * 0.01f);
	auto& standby = m_sources.at((m_current_stream + 1) % m_sources.size());
	standby.set_gain((1.0f - ratio) * volume * 0.01f);

	if (m_cross_fade->elapsed >= m_cross_fade->total) { m_cross_fade.reset(); }
}
} // namespace le::audio
