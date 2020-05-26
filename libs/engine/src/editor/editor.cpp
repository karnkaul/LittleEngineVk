// TODO: REMOVE
#if !defined(LEVK_EDITOR)
#define LEVK_EDITOR
#endif
#if !defined(LEVK_USE_IMGUI)
#define LEVK_USE_IMGUI
#endif

#include <deque>
#include <mutex>
#include "core/log_config.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/editor/editor.hpp"
#if defined(LEVK_EDITOR)
#if defined(LEVK_USE_IMGUI)
#include <imgui.h>
#endif
#include "gfx/ext_gui.hpp"

namespace le
{
struct Editor final
{
};
static std::string const s_tName = utils::tName<Editor>();

namespace
{
bool g_bInit = false;
log::OnLog g_onLogChain = nullptr;
std::mutex g_logMutex;

struct LogEntry final
{
	std::string text;
#if defined(LEVK_USE_IMGUI)
	ImVec4 imColour;
#endif
	log::Level level;
	Colour colour;
};

size_t g_maxLogEntries = 500;
std::deque<LogEntry> g_logEntries;

glm::ivec2 const g_minDim = glm::ivec2(100, 100);

#if defined(LEVK_USE_IMGUI)
bool g_bAutoScroll = true;
#endif

Colour fromLevel(log::Level level)
{
	switch (level)
	{
	case log::Level::eError:
		return colours::red;
	case log::Level::eWarning:
		return colours::yellow;
	case log::Level::eDebug:
		return Colour(0x888888ff);
	default:
		return colours::white;
	}
}

#if defined(LEVK_USE_IMGUI)
ImVec4 fromColour(Colour colour)
{
	return ImVec4(colour.r.toF32(), colour.g.toF32(), colour.b.toF32(), colour.a.toF32());
}
#endif

void guiLog(std::string_view text, log::Level level)
{
	LogEntry entry;
	entry.text = std::string(text);
	entry.colour = fromLevel(level);
	entry.level = level;
#if defined(LEVK_USE_IMGUI)
	entry.imColour = fromColour(entry.colour);
#endif
	std::scoped_lock<std::mutex> lock(g_logMutex);
	g_logEntries.push_back(std::move(entry));
	while (g_logEntries.size() > g_maxLogEntries)
	{
		g_logEntries.pop_front();
	}
	if (g_onLogChain)
	{
		g_onLogChain(text, level);
	}
}

void clearLog()
{
	g_logEntries.clear();
}

void drawLeftPanel([[maybe_unused]] glm::ivec2 const& fbSize, glm::ivec2 const& panelSize)
{
	if (panelSize.x < g_minDim.x || panelSize.y < g_minDim.y)
	{
		return;
	}
#if defined(LEVK_USE_IMGUI)
	static s32 const s_xPad = 2;
	static s32 const s_dy = 2;

	// clang-format off
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)(panelSize.x - s_xPad - s_xPad), (f32)(panelSize.y - s_dy)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(s_xPad, (f32)s_dy), ImGuiCond_Always);
	if (!ImGui::Begin("Left", nullptr, flags))
	{
		ImGui::End();
		return;
	}
	char const* szPlayPause = editor::g_bTickGame ? "Pause" : "Play";
	if (ImGui::Button(szPlayPause))
	{
		editor::g_bTickGame = !editor::g_bTickGame;
		LOG_I("[{}] {}", s_tName, editor::g_bTickGame ? "Resumed" : "Paused");
	}
	ImGui::End();
	// clang-format on
#endif
	return;
}

void drawRightPanel(glm::ivec2 const& fbSize, glm::ivec2 const& panelSize)
{
	if (panelSize.x < g_minDim.x || panelSize.y < g_minDim.y)
	{
		return;
	}
#if defined(LEVK_USE_IMGUI)
	static s32 const s_xPad = 2;
	static s32 const s_dy = 2;

	// clang-format off
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)(panelSize.x - s_xPad - s_xPad), (f32)(panelSize.y - s_dy)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2((f32)(fbSize.x - panelSize.x + s_xPad), (f32)s_dy), ImGuiCond_Always);
	if (!ImGui::Begin("Right", nullptr, flags))
	{
		ImGui::End();
		return;
	}
	char const* szPlayPause = editor::g_bTickGame ? "Pause" : "Play";
	if (ImGui::Button(szPlayPause))
	{
		editor::g_bTickGame = !editor::g_bTickGame;
		LOG_I("[{}] {}", s_tName, editor::g_bTickGame ? "Resumed" : "Paused");
	}
	ImGui::End();
	// clang-format on
#endif
	return;
}

void drawLog(glm::ivec2 const& fbSize, s32 logHeight)
{
#if defined(LEVK_USE_IMGUI)
	// clang-format off
	static s32 const s_yPad = 3;
	static s32 s_logLevel = 0;
	bool bClear = false;

	if (logHeight - s_yPad <= g_minDim.y)
	{
		return;
	}
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)fbSize.x, (f32)(logHeight - s_yPad)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(0.0f, (f32)(fbSize.y - logHeight + s_yPad)), ImGuiCond_Always);
	if (!ImGui::Begin("Log", nullptr, flags))
	{
		ImGui::End();
		return;
	}
	// Widgets
	{
		ImGui::SameLine();
		bClear = ImGui::Button("Clear"); ImGui::SameLine();
		ImGui::Checkbox("Auto-scroll", &g_bAutoScroll);
	}
	{
		ImGui::SameLine();
		ImGui::RadioButton("All", &s_logLevel, 0); ImGui::SameLine();
		ImGui::RadioButton("Info", &s_logLevel, 1); ImGui::SameLine();
		ImGui::RadioButton("Warning", &s_logLevel, 2); ImGui::SameLine();
		ImGui::RadioButton("Error", &s_logLevel, 3);
	}
	{
		ImGui::SameLine();
		// Arrow buttons with Repeater
		f32 const spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		s32 counter = (s32)g_maxLogEntries;
		ImGui::PushButtonRepeat(true);
		if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { counter--; }
		ImGui::SameLine(0.0f, spacing);
		if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { counter++; }
		ImGui::PopButtonRepeat();
		ImGui::SameLine();
		ImGui::Text("%d", counter);
		g_maxLogEntries = (size_t)std::clamp(counter, 10, 1000);
	}
	ImGui::Separator();
	{
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		if (bClear)
		{
			clearLog();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		std::scoped_lock<std::mutex> lock(g_logMutex);
		for (auto const& entry : g_logEntries)
		{
			if ((s32)entry.level >= s_logLevel)
			{
				ImGui::TextColored(fromColour(entry.colour), "%s", entry.text.data());
			}
		}
		ImGui::PopStyleVar();
		if (g_bAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		{
			ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();
	}
	ImGui::End();
	// clang-format on
#endif
}
} // namespace

bool editor::init()
{
	if (!g_bInit && gfx::ext_gui::isInit())
	{
		g_onLogChain = log::g_onLog;
		log::g_onLog = &guiLog;
		return g_bInit = true;
	}
	return false;
}

void editor::deinit()
{
	if (g_bInit)
	{
		log::g_onLog = g_onLogChain;
		g_onLogChain = nullptr;
		g_bInit = false;
	}
	return;
}

bool editor::render(gfx::ScreenRect const& scene, glm::ivec2 const& fbSize)
{
	if (gfx::ext_gui::isInit() && fbSize.x > 0 && fbSize.y > 0)
	{
		auto const logHeight = fbSize.y - (s32)(scene.bottom * (f32)fbSize.y);
		glm::ivec2 const leftPanelSize = {(s32)(scene.left * (f32)fbSize.x), fbSize.y - logHeight};
		glm::ivec2 const rightPanelSize = {fbSize.x - (s32)(scene.right * (f32)fbSize.x), fbSize.y - logHeight};
		drawLog(fbSize, logHeight);
		drawLeftPanel(fbSize, leftPanelSize);
		drawRightPanel(fbSize, rightPanelSize);
		return true;
	}
	return false;
}
} // namespace le
#endif
