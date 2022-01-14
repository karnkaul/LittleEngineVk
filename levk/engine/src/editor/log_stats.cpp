#include <levk/engine/editor/log_stats.hpp>
#include <levk/engine/editor/types.hpp>

#if defined(LEVK_USE_IMGUI)
#include <imgui.h>
#include <core/array_map.hpp>
#include <core/colour.hpp>
#include <core/services.hpp>
#include <dumb_log/pipe.hpp>
#include <ktl/async/kmutex.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/utils/engine_stats.hpp>
#endif

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
struct LogText {
	std::string text;
	ImVec4 colour;
	LogLevel level;
};

constexpr EnumArray<LogLevel, Colour, 4> logColour = {Colour(0x666666ff), Colour(0xccccccff), Colour(0xdddd22ff), Colour(0xff1111ff)};

ImVec4 imvec4(Colour c) noexcept { return {c.r.toF32(), c.g.toF32(), c.b.toF32(), c.a.toF32()}; }

ktl::strict_tmutex<std::deque<LogText>> g_logs;

struct LogPipe : dlog::pipe {
	void operator()(dlog::level level, std::string_view text) const override {
		ktl::klock lock(g_logs);
		ImVec4 const colour = imvec4(logColour[level]);
		lock->push_front({std::string(text), colour, level});
		while (lock->size() > LogStats::s_maxLines) { lock->pop_back(); }
	}
};

struct FrameTime {
	Span<f32 const> samples;
	f32 average{};
	u32 rate{};
};

void drawLog(glm::vec2 fbSize, f32 logHeight, FrameTime ft) {
	static f32 const s_yPad = 3.0f;
	if (logHeight - s_yPad <= 50.0f) { return; }

	static std::array<char, 64> filter = {0};
	bool bClear = false;
	std::string_view logFilter;
	static constexpr ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowSize(ImVec2(fbSize.x, logHeight - s_yPad), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(0.0f, fbSize.y - logHeight + s_yPad), ImGuiCond_Always);
	if (ImGui::Begin("##LogStats", nullptr, flags)) {
		// Frame time
		{
			static constexpr s64 s_minTCounter = 1, s_maxTCounter = 20, scale = 25;
			s64 ftCount = (s64)LogStats::s_frameTimeCount / scale;
			TWidget<std::pair<s64, s64>> st(CStr<32>("ft count: %i", ftCount * scale), ftCount, s_minTCounter, s_maxTCounter, 1);
			LogStats::s_frameTimeCount = (std::size_t)ftCount * scale;
			f32 const ftime = ft.samples.empty() ? 0.0f : ft.samples.back();
			auto const overlay = CStr<32>("%.4fms (avg of %u)", ft.average, ft.samples.size());
			auto const title = CStr<32>("[%.3fms] [%u] FPS", ftime, ft.rate);
			Styler s(Style::eSameLine);
			ImGui::PlotLines(title.data(), ft.samples.data(), (s32)ft.samples.size(), 0, overlay.data());
			s(Style::eSeparator);
		}
		// TWidgets
		{
			Text title("Log");
			Styler s(Style::eSameLine);
			bClear = static_cast<bool>(Button("Clear"));
			s();
			if (ImGui::GetIO().MouseWheel > 0.0f) { LogStats::s_autoScroll = false; }
			ImGui::Checkbox("Auto-scroll", &LogStats::s_autoScroll);
		}
		{
			Styler s(Style::eSameLine);
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("Filter", filter.data(), filter.size());
			logFilter = filter.data();
		}
		{
			static constexpr std::array levels = {"All"sv, "Info"sv, "Warning"sv, "Error"sv};
			s32 logLevel = (s32)LogStats::s_logLevel;
			LogStats::s_logLevel = (LogLevel)Radio(levels, logLevel).select;
		}
		{
			Styler s(Style::eSameLine);
			static constexpr s64 s_minTCounter = 1, s_maxTCounter = 20, scale = 100;
			s64 lineCount = (s64)LogStats::s_lineCount / scale;
			TWidget<std::pair<s64, s64>> st(CStr<32>("line count: %i", lineCount * scale), lineCount, s_minTCounter, s_maxTCounter, 1);
			LogStats::s_lineCount = (std::size_t)lineCount * scale;
		}
		{
			Styler s(Style::eSeparator);
			Pane pane("scrolling", {}, false, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			std::vector<Ref<LogText const>> filtered;
			filtered.reserve(LogStats::s_lineCount);
			ktl::klock lock(g_logs);
			if (bClear) { lock->clear(); }
			for (auto const& entry : *lock) {
				if (entry.level >= LogStats::s_logLevel && (logFilter.empty() || entry.text.find(logFilter) != std::string::npos)) {
					filtered.push_back(entry);
					if (filtered.size() == LogStats::s_lineCount) { break; }
				}
			}
			for (auto it = filtered.rbegin(); it != filtered.rend(); ++it) {
				LogText const& entry = *it;
				ImGui::TextColored(entry.colour, "%s", entry.text.data());
			}
			ImGui::PopStyleVar();
			if (LogStats::s_autoScroll && ImGui::GetScrollY() <= ImGui::GetScrollMaxY()) { ImGui::SetScrollHereY(1.0f); }
		}
	}
	ImGui::End();
}

} // namespace
#endif

LogStats::LogStats() {
#if defined(LEVK_USE_IMGUI)
	m_handle = dlog::pipe::attach_pipe<LogPipe>();
#endif
}

void LogStats::operator()([[maybe_unused]] glm::vec2 fbSize, [[maybe_unused]] f32 height) {
#if defined(LEVK_USE_IMGUI)
	if (auto eng = Services::find<Engine>()) {
		auto const& stats = eng->stats().frame;
		if (stats.dt != Time_s()) { m_frameTime.fts.push_back(stats.dt); }
		while (m_frameTime.fts.size() > s_frameTimeCount) { m_frameTime.fts.pop_front(); }
		m_frameTime.samples.clear();
		m_frameTime.samples.reserve(s_frameTimeCount);
		stdch::duration<f32, std::milli> avg{};
		for (Time_s const ft : m_frameTime.fts) {
			avg += ft;
			m_frameTime.samples.push_back(time::cast<decltype(avg)>(ft).count());
		}
		avg /= (f32)m_frameTime.fts.size();
		u32 const rate = stats.rate == 0 ? (u32)stats.count : stats.rate;
		drawLog(fbSize, height, {m_frameTime.samples, avg.count(), rate});
	}
#endif
}
} // namespace le::edi
