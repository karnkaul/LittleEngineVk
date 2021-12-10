#pragma once
#include <core/log.hpp>
#include <core/std_types.hpp>
#include <glm/vec2.hpp>

#if defined(LEVK_USE_IMGUI)
#include <core/time.hpp>
#include <dumb_log/pipe.hpp>
#include <deque>
#endif

namespace le::edi {
class LogStats {
  public:
	inline static std::size_t s_maxLines = 5000;
	inline static std::size_t s_lineCount = 200;
	inline static bool s_autoScroll = true;
	inline static LogLevel s_logLevel = LogLevel::debug;

	inline static std::size_t s_frameTimeCount = 200;

	LogStats();

	void operator()(glm::vec2 fbSize, f32 lheight);

#if defined(LEVK_USE_IMGUI)
  private:
	struct {
		std::deque<Time_s> fts;
		std::vector<f32> samples;
	} m_frameTime;
	dlog::pipe::handle m_handle;
#endif
};
} // namespace le::edi
