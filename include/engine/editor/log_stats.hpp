#pragma once
#include <core/std_types.hpp>
#include <dumb_log/log.hpp>
#include <glm/vec2.hpp>

#if defined(LEVK_USE_IMGUI)
#include <deque>
#include <core/time.hpp>
#endif

namespace le::edi {
class LogStats {
  public:
	inline static std::size_t s_maxLines = 5000;
	inline static std::size_t s_lineCount = 200;
	inline static bool s_autoScroll = true;
	inline static dl::level s_logLevel = dl::level::debug;

	inline static std::size_t s_frameTimeCount = 200;

	LogStats();

	void operator()(glm::vec2 fbSize, f32 lheight);

#if defined(LEVK_USE_IMGUI)
  private:
	struct {
		std::deque<Time_s> fts;
		std::vector<f32> samples;
	} m_frameTime;
	time::Point m_elapsed;
	dl::config::on_log::token m_token;
#endif
};
} // namespace le::edi
