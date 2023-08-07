#pragma once
#include <capo/device.hpp>
#include <capo/pcm.hpp>
#include <le/core/not_null.hpp>
#include <le/core/time.hpp>
#include <array>

namespace le::audio {
using capo::Pcm;
using capo::StreamSource;

class Music {
  public:
	explicit Music(NotNull<capo::Device*> device);

	[[nodiscard]] auto get_music_state() const -> capo::State { return m_sources.at(m_current_stream).state(); }
	[[nodiscard]] auto is_music_playing() const -> bool { return get_music_state() == capo::State::ePlaying; }

	auto play(NotNull<Pcm const*> pcm, Duration cross_fade = 1s) -> void;
	auto stop(Duration cross_fade = 1s) -> void;

	auto tick(Duration dt) -> void;

	float volume{80.0f};

  private:
	struct CrossFade {
		Duration total{};
		Duration elapsed{};
	};

	std::array<StreamSource, 2> m_sources{};
	std::size_t m_current_stream{};
	std::optional<CrossFade> m_cross_fade{};
};
} // namespace le::audio
