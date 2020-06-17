#include <deque>
#include <mutex>
#include <core/log_config.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <editor/editor.hpp>
#if defined(LEVK_EDITOR)
#if defined(LEVK_USE_IMGUI)
#include <imgui.h>
#endif
#include <core/maths.hpp>
#include <glm/gtc/quaternion.hpp>
#include <gfx/ext_gui.hpp>
#include <engine/game/world.hpp>
#include <engine/window/window.hpp>
#include <window/window_impl.hpp>
#include <engine/gfx/renderer.hpp>
#include <engine/window/input_types.hpp>

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

struct
{
	WindowID window;
	OnInput::Token inputToken;
	OnFocus::Token focusToken;
	bool enabled = false;
	bool bAltPressed = false;
} g_data;

struct LogEntry final
{
	std::string text;
	ImVec4 imColour;
	log::Level level;
};

size_t g_maxLogEntries = 500;
std::deque<LogEntry> g_logEntries;

glm::ivec2 const g_minDim = glm::ivec2(100, 100);

struct
{
	Entity entity;
	Transform* pTransform = nullptr;
} g_inspecting;
World* g_pWorld = nullptr;

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
	entry.level = level;
	entry.imColour = fromColour(fromLevel(level));
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

bool isDifferent(glm::vec3 const& lhs, glm::vec3 const& rhs)
{
	return !maths::equals(lhs.x, rhs.x, 0.001f) || !maths::equals(lhs.y, rhs.y, 0.001f) || !maths::equals(lhs.z, rhs.z, 0.001f);
}

void walkGraph(Transform& root, World::EMap const& emap, Registry& registry)
{
	static ImGuiTreeNodeFlags const baseFlags =
		ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	auto const children = root.children();
	auto search = emap.find(&root);
	if (search != emap.end())
	{
		ImGuiTreeNodeFlags nodeFlags = baseFlags;
		auto entity = search->second;
		if (g_inspecting.entity == entity)
		{
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}
		auto pTransform = search->first;
		bool const bLeaf = pTransform->children().empty();
		if (bLeaf)
		{
			nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}
		bool const bNodeOpen = ImGui::TreeNodeEx(registry.entityName(entity).data(), nodeFlags) && !bLeaf;
		if (ImGui::IsItemClicked())
		{
			if (g_inspecting.entity == entity)
			{
				g_inspecting = {};
			}
			else
			{
				g_inspecting.entity = entity;
				g_inspecting.pTransform = pTransform;
			}
		}
		if (bNodeOpen)
		{
			for (auto pChild : pTransform->children())
			{
				walkGraph(*pChild, emap, registry);
			}
			ImGui::TreePop();
		}
	}
}

void drawLeftPanel(glm::ivec2 const& panelSize)
{
	if (panelSize.x < g_minDim.x || panelSize.y < g_minDim.y)
	{
		return;
	}
	static s32 const s_xPad = 2;
	static s32 const s_dy = 2;

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)(panelSize.x - s_xPad - s_xPad), (f32)(panelSize.y - s_dy)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(s_xPad, (f32)s_dy), ImGuiCond_Always);
	if (!ImGui::Begin("Scene", nullptr, flags))
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
	if (g_pWorld)
	{
		for (auto pTransform : g_pWorld->m_root.children())
		{
			walkGraph(*pTransform, g_pWorld->m_transformToEntity, g_pWorld->registry());
		}
	}
	ImGui::End();
	return;
}

void drawRightPanel(glm::ivec2 const& fbSize, glm::ivec2 const& panelSize)
{
	if (panelSize.x < g_minDim.x || panelSize.y < g_minDim.y)
	{
		return;
	}
	static s32 const s_xPad = 2;
	static s32 const s_dy = 2;

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)(panelSize.x - s_xPad - s_xPad), (f32)(panelSize.y - s_dy)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2((f32)(fbSize.x - panelSize.x + s_xPad), (f32)s_dy), ImGuiCond_Always);
	if (!ImGui::Begin("Inspector", nullptr, flags))
	{
		ImGui::End();
		return;
	}
	if (g_pWorld && g_inspecting.entity != Entity() && g_inspecting.pTransform)
	{
		auto& registry = g_pWorld->registry();
		ImGui::LabelText("", "%s", registry.entityName(g_inspecting.entity).data());
		bool const bEnabled = registry.isEnabled(g_inspecting.entity);
		char const* szToggle = bEnabled ? "Disable" : "Enable";
		if (ImGui::Button(szToggle))
		{
			registry.setEnabled(g_inspecting.entity, !bEnabled);
		}
		ImGui::SameLine();
		if (ImGui::Button("Destroy"))
		{
			registry.destroyEntity(g_inspecting.entity);
			g_inspecting = {};
		}
		else
		{
			auto pos = g_inspecting.pTransform->position();
			auto const& orn = g_inspecting.pTransform->orientation();
			auto rot = glm::eulerAngles(orn);
			auto const rotOrg = rot;
			ImGui::DragFloat3("Pos", &pos.x, 0.1f);
			if (isDifferent(pos, g_inspecting.pTransform->position()))
			{
				g_inspecting.pTransform->setPosition(pos);
			}
			ImGui::DragFloat3("Orn", &rot.x, 0.01f);
			if (isDifferent(rot, rotOrg))
			{
				g_inspecting.pTransform->setOrientation(glm::quat(rot));
			}
		}
	}
	ImGui::End();
	return;
}

void drawLog(glm::ivec2 const& fbSize, s32 logHeight)
{
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
		bClear = ImGui::Button("Clear");
		ImGui::SameLine();
		ImGui::Checkbox("Auto-scroll", &g_bAutoScroll);
	}
	{
		ImGui::SameLine();
		ImGui::RadioButton("All", &s_logLevel, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Info", &s_logLevel, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Warning", &s_logLevel, 2);
		ImGui::SameLine();
		ImGui::RadioButton("Error", &s_logLevel, 3);
	}
	{
		ImGui::SameLine();
		// Arrow buttons with Repeater
		f32 const spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		s32 counter = (s32)g_maxLogEntries;
		ImGui::PushButtonRepeat(true);
		if (ImGui::ArrowButton("##left", ImGuiDir_Left))
		{
			counter--;
		}
		ImGui::SameLine(0.0f, spacing);
		if (ImGui::ArrowButton("##right", ImGuiDir_Right))
		{
			counter++;
		}
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
				ImGui::TextColored(entry.imColour, "%s", entry.text.data());
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
}
} // namespace

bool editor::init(WindowID editorWindow)
{
	if (!g_bInit && gfx::ext_gui::isInit())
	{
		g_onLogChain = log::g_onLog;
		log::g_onLog = &guiLog;
		g_data.window = editorWindow;
		g_data.inputToken = Window::registerInput(
			[](Key key, Action action, Mods mods) {
				if (key == Key::eE && action == Action::eRelease && mods & Mods::eCONTROL)
				{
					g_data.enabled = !g_data.enabled;
				}
				if ((key == Key::eLeftAlt || key == Key::eRightAlt) && (action == Action::ePress || action == Action::eRelease))
				{
					g_data.bAltPressed = action == Action::ePress;
				}
			},
			editorWindow);
		if (auto pWindow = WindowImpl::windowImpl(g_data.window))
		{
			g_data.focusToken = pWindow->m_pWindow->registerFocus([](bool) { g_data.bAltPressed = false; });
		}
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
		g_data = {};
	}
	return;
}

gfx::ScreenRect editor::tick([[maybe_unused]] Time dt)
{
	static auto const smol = glm::vec2(0.66f);
	auto pWindow = WindowImpl::windowImpl(g_data.window);
	if (g_data.enabled && pWindow && pWindow->isOpen())
	{
		glm::vec2 centre = glm::vec2(0.5f * (f32)pWindow->windowSize().x, 0.0f);
		auto const fbSize = pWindow->framebufferSize();
		if (g_data.bAltPressed)
		{
			centre = pWindow->cursorPos();
		}
		auto const scene = pWindow->m_pWindow->renderer().clampToView(centre, smol);
		if (gfx::ext_gui::isInit() && fbSize.x > 0 && fbSize.y > 0)
		{
			auto const logHeight = fbSize.y - (s32)(scene.bottom * (f32)fbSize.y);
			glm::ivec2 const leftPanelSize = {(s32)(scene.left * (f32)fbSize.x), fbSize.y - logHeight};
			glm::ivec2 const rightPanelSize = {fbSize.x - (s32)(scene.right * (f32)fbSize.x), fbSize.y - logHeight};
			auto pActive = World::active();
			if (!pActive || pActive != g_pWorld)
			{
				g_inspecting = {};
			}
			g_pWorld = pActive;
			drawLog(fbSize, logHeight);
			drawLeftPanel(leftPanelSize);
			drawRightPanel(fbSize, rightPanelSize);
			return scene;
		}
	}
	return {};
}
} // namespace le
#endif
