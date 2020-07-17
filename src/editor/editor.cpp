#include <deque>
#include <mutex>
#include <core/log_config.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <editor/editor.hpp>
#if defined(LEVK_EDITOR)
#include <fmt/format.h>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <core/maths.hpp>
#include <core/utils.hpp>
#include <gfx/ext_gui.hpp>
#include <engine/assets/resources.hpp>
#include <engine/game/world.hpp>
#include <engine/window/window.hpp>
#include <window/window_impl.hpp>
#include <engine/gfx/font.hpp>
#include <engine/gfx/mesh.hpp>
#include <engine/gfx/model.hpp>
#include <engine/gfx/shader.hpp>
#include <engine/gfx/texture.hpp>
#include <engine/gfx/renderer.hpp>
#include <engine/window/input_types.hpp>

namespace le
{
using namespace input;

struct Editor final
{
};
static std::string const s_tName = utils::tName<Editor>();

namespace
{
bool g_bInit = false;
io::OnLog g_onLogChain = nullptr;
Lockable<std::mutex> g_logMutex;

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
	io::Level level;
};

std::size_t g_maxLogEntries = 500;
std::deque<LogEntry> g_logEntries;

glm::ivec2 const g_minDim = glm::ivec2(100, 100);

struct
{
	Entity entity;
	Transform* pTransform = nullptr;
	struct
	{
		bool bSelectID = false;
		bool bSelectMat = false;
		bool bSelectDiffuse = false;
	} mesh;
	struct
	{
		bool bSelectID = false;
	} model;
} g_inspecting;
World* g_pWorld = nullptr;

#if defined(LEVK_USE_IMGUI)
bool g_bAutoScroll = true;
#endif

Colour fromLevel(io::Level level)
{
	switch (level)
	{
	case io::Level::eError:
		return colours::red;
	case io::Level::eWarning:
		return colours::yellow;
	case io::Level::eDebug:
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

void guiLog(std::string_view text, io::Level level)
{
	LogEntry entry;
	entry.text = std::string(text);
	entry.level = level;
	entry.imColour = fromColour(fromLevel(level));
	auto lock = g_logMutex.lock();
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
	static ImGuiTreeNodeFlags const baseFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
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

template <typename T>
void listAssets(std::string_view tabName)
{
	if (ImGui::BeginTabItem(tabName.data()))
	{
		auto assets = Resources::inst().loaded<T>();
		static s32 selected = -1;
		for (std::size_t i = 0; i < assets.size(); ++i)
		{
			auto pAsset = assets.at(i);
			if (ImGui::Selectable(pAsset->m_id.generic_string().data(), selected == (s32)i))
			{
				selected = (s32)i;
			}
		}
		ImGui::EndTabItem();
	}
}

void resourcesWindow(glm::vec2 const& pos, glm::vec2 const& size)
{
	static bool s_bAssets = false;
	s_bAssets |= ImGui::Button("Resources");
	if (s_bAssets)
	{
		ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Loaded Assets", &s_bAssets, ImGuiWindowFlags_NoSavedSettings))
		{
			if (ImGui::BeginTabBar("Resources"))
			{
				listAssets<gfx::Model>("Models");
				listAssets<gfx::Mesh>("Meshes");
				listAssets<gfx::Font>("Fonts");
				listAssets<gfx::Texture>("Textures");
				listAssets<gfx::Material>("Materials");
				listAssets<gfx::Sampler>("Samplers");
				listAssets<gfx::Shader>("Shaders");
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}
}

template <typename T>
bool dummy(T&)
{
	return true;
}

template <typename T, typename F, typename F2>
void inspectAsset(T* pAsset, std::string_view selector, bool& out_bSelect, std::initializer_list<bool*> unselect, F onSelected, F2 filter, glm::vec2 const& pos,
				  glm::vec2 const& size, bool bNone = true)
{
	static ImGuiTreeNodeFlags const flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth
											| ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	ImGui::TreeNodeEx(pAsset ? pAsset->m_id.generic_string().data() : "[None]", flags);
	if (ImGui::IsItemClicked() || out_bSelect)
	{
		out_bSelect = true;
		for (auto pBool : unselect)
		{
			*pBool = false;
		}
		ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(selector.data(), &out_bSelect, ImGuiWindowFlags_NoSavedSettings))
		{
			if (bNone && ImGui::Selectable("[None]"))
			{
				onSelected(nullptr);
				out_bSelect = false;
			}
			auto assets = Resources::inst().loaded<T>();
			for (auto pAsset : assets)
			{
				if (!filter || filter(*pAsset))
				{
					if (ImGui::Selectable(pAsset->m_id.generic_string().data()))
					{
						onSelected(pAsset);
						out_bSelect = false;
					}
				}
			}
		}
		ImGui::End();
	}
}

void inspectMaterial(gfx::Mesh& out_mesh, std::size_t idx, glm::vec2 const& pos, glm::vec2 const& size)
{
	if (ImGui::TreeNode(fmt::format("Material{}", idx).data()))
	{
		inspectAsset<gfx::Material>(
			out_mesh.m_material.pMaterial, "Loaded Materials", g_inspecting.mesh.bSelectMat, {&g_inspecting.mesh.bSelectDiffuse, &g_inspecting.mesh.bSelectID},
			[&out_mesh](gfx::Material* pMat) { out_mesh.m_material.pMaterial = pMat; }, &dummy<gfx::Material>, pos, size, false);
		inspectAsset<gfx::Texture>(
			out_mesh.m_material.pDiffuse, "Loaded Textures", g_inspecting.mesh.bSelectDiffuse, {&g_inspecting.mesh.bSelectID, &g_inspecting.mesh.bSelectMat},
			[&out_mesh](gfx::Texture* pTex) { out_mesh.m_material.pDiffuse = pTex; }, [](gfx::Texture& tex) { return tex.m_type == gfx::Texture::Type::e2D; },
			pos, size);
		bool bOut = out_mesh.m_material.flags[gfx::Material::Flag::eDropColour];
		ImGui::Checkbox("Drop Colour", &bOut);
		out_mesh.m_material.flags[gfx::Material::Flag::eDropColour] = bOut;
		bOut = out_mesh.m_material.flags[gfx::Material::Flag::eOpaque];
		ImGui::Checkbox("Opaque", &bOut);
		out_mesh.m_material.flags[gfx::Material::Flag::eOpaque] = bOut;
		bOut = out_mesh.m_material.flags[gfx::Material::Flag::eTextured];
		ImGui::Checkbox("Textured", &bOut);
		out_mesh.m_material.flags[gfx::Material::Flag::eTextured] = bOut;
		bOut = out_mesh.m_material.flags[gfx::Material::Flag::eLit];
		ImGui::Checkbox("Lit", &bOut);
		out_mesh.m_material.flags[gfx::Material::Flag::eLit] = bOut;
		ImGui::TreePop();
	}
}

void entityInspector(glm::vec2 const& pos, glm::vec2 const& size)
{
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
			auto posn = g_inspecting.pTransform->position();
			auto const& orn = g_inspecting.pTransform->orientation();
			auto rot = glm::eulerAngles(orn);
			auto const rotOrg = rot;
			ImGui::DragFloat3("Pos", &posn.x, 0.1f);
			if (isDifferent(posn, g_inspecting.pTransform->position()))
			{
				g_inspecting.pTransform->setPosition(posn);
			}
			ImGui::DragFloat3("Orn", &rot.x, 0.01f);
			if (isDifferent(rot, rotOrg))
			{
				g_inspecting.pTransform->setOrientation(glm::quat(rot));
			}
			ImGui::Separator();

			auto pTMesh = registry.component<TAsset<gfx::Mesh>>(g_inspecting.entity);
			auto pMesh = pTMesh ? pTMesh->get() : nullptr;
			if (pTMesh)
			{
				if (ImGui::TreeNode("Mesh"))
				{
					inspectAsset<gfx::Mesh>(
						pMesh, "Loaded Meshes", g_inspecting.mesh.bSelectID, {&g_inspecting.mesh.bSelectDiffuse, &g_inspecting.mesh.bSelectMat},
						[pTMesh](gfx::Mesh const* pMesh) { pTMesh->id = pMesh ? pMesh->m_id : stdfs::path(); }, &dummy<gfx::Mesh>, pos, size);

					if (pMesh)
					{
						inspectMaterial(*pMesh, 0, pos, size);
					}
					ImGui::TreePop();
				}
			}
			auto pTModel = registry.component<TAsset<gfx::Model>>(g_inspecting.entity);
			auto pModel = pTModel ? pTModel->get() : nullptr;
			if (pTModel)
			{
				if (ImGui::TreeNode("Model"))
				{
					inspectAsset<gfx::Model>(
						pModel, "Loaded Models", g_inspecting.model.bSelectID, {},
						[pTModel](gfx::Model const* pModel) { pTModel->id = pModel ? pModel->m_id : stdfs::path(); }, &dummy<gfx::Model>, pos, size);
					static std::deque<gfx::Mesh> s_empty;
					auto& meshes = pModel ? pModel->loadedMeshes() : s_empty;
					std::size_t idx = 0;
					for (auto& mesh : meshes)
					{
						inspectMaterial(mesh, idx++, pos, size);
					}
					ImGui::TreePop();
				}
			}
		}
	}
}

void playButton()
{
	char const* szPlayPause = editor::g_bTickGame ? "Pause" : "Play";
	if (ImGui::Button(szPlayPause))
	{
		editor::g_bTickGame = !editor::g_bTickGame;
		LOG_I("[{}] {}", s_tName, editor::g_bTickGame ? "Resumed" : "Paused");
	}
}

void presentModeDropdown()
{
	if (auto pWindow = WindowImpl::windowImpl(g_data.window))
	{
		static std::array<std::string_view, 4> const s_presentModes = {"Off", "Triple Buffer", "Double Buffer", "Double Buffer (Relaxed)"};
		auto presentMode = pWindow->presentMode();
		if (ImGui::BeginCombo("Vsync", s_presentModes[(std::size_t)presentMode].data()))
		{
			auto const& presentModes = pWindow->m_presentModes;
			static std::size_t s_selected = 100;
			std::size_t previous = s_selected;
			for (std::size_t i = 0; i < presentModes.size() && i < s_presentModes.size(); ++i)
			{
				if (s_selected > s_presentModes.size() && presentMode == presentModes.at(i))
				{
					s_selected = i;
					previous = s_selected;
				}
				bool const bSelected = s_selected == i;
				auto const iMode = presentModes.at(i);
				if (ImGui::Selectable(s_presentModes[(std::size_t)iMode].data(), bSelected))
				{
					s_selected = i;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			if (previous != s_selected)
			{
				auto const newPresentMode = presentModes.at(s_selected);
				if (pWindow->setPresentMode(newPresentMode))
				{
					LOG_I("[{}] Switched present mode from [{}] to [{}]", s_tName, s_presentModes[(std::size_t)presentMode],
						  s_presentModes[(std::size_t)newPresentMode]);
				}
				else
				{
					LOG_E("[{}] Failed to switch present mode!", s_tName);
				}
			}
			ImGui::EndCombo();
		}
	}
}

void worldSelectDropdown()
{
	World* pNewWorld = nullptr;
	auto worldName = std::string(g_pWorld ? g_pWorld->name() : "[None]");
	utils::removeNamesapces(worldName);
	if (g_pWorld && g_pWorld->isBusy())
	{
		ImGui::LabelText("", "%s (Busy)", worldName.data());
	}
	else if (ImGui::BeginCombo("Worlds", worldName.data()))
	{
		auto const worlds = World::allWorlds();
		static std::size_t s_selected = 0;
		for (std::size_t i = 0; i < worlds.size(); ++i)
		{
			bool const bSelected = s_selected == i;
			auto name = std::string(worlds.at(i)->name());
			utils::removeNamesapces(name);
			if (ImGui::Selectable(name.data(), bSelected))
			{
				s_selected = i;
				pNewWorld = worlds.at(i);
			}
			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (pNewWorld && pNewWorld != g_pWorld)
	{
		World::loadWorld(pNewWorld->id());
	}
}

void drawLeftPanel([[maybe_unused]] glm::ivec2 const& fbSize, glm::ivec2 const& panelSize)
{
	if (panelSize.x < g_minDim.x || panelSize.y < g_minDim.y)
	{
		return;
	}
	static s32 const s_xPad = 2;
	static s32 const s_dy = 2;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)(panelSize.x - s_xPad - s_xPad), (f32)(panelSize.y - s_dy)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(s_xPad, (f32)s_dy), ImGuiCond_Always);
	if (ImGui::Begin("Inspector", nullptr, flags))
	{
		resourcesWindow({(f32)(panelSize.x + s_xPad), 200.0f}, {500.0f, 300.0f});
		static bool s_bImGuiDemo = false;
		s_bImGuiDemo |= ImGui::Button("ImGui Demo");
		if (s_bImGuiDemo)
		{
			ImGui::ShowDemoWindow(&s_bImGuiDemo);
		}
		ImGui::Separator();
		entityInspector({(f32)(panelSize.x + s_xPad * 4), 200.0f}, {300.0f, 200.0f});
	}
	ImGui::End();
	return;
}

void drawRightPanel([[maybe_unused]] glm::ivec2 const& fbSize, glm::ivec2 const& panelSize)
{
	if (panelSize.x < g_minDim.x || panelSize.y < g_minDim.y)
	{
		return;
	}
	static s32 const s_xPad = 2;
	static s32 const s_dy = 2;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)(panelSize.x - s_xPad - s_xPad), (f32)(panelSize.y - s_dy)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2((f32)(fbSize.x - panelSize.x + s_xPad), (f32)s_dy), ImGuiCond_Always);
	if (ImGui::Begin("Scene", nullptr, flags))
	{
		playButton();
		if (ImGui::BeginTabBar("Right"))
		{
			if (ImGui::BeginTabItem("Scene"))
			{
				if (g_pWorld)
				{
					for (auto pTransform : g_pWorld->m_root.children())
					{
						walkGraph(*pTransform, g_pWorld->m_transformToEntity, g_pWorld->registry());
					}
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Options"))
			{
				presentModeDropdown();
				worldSelectDropdown();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
	return;
}

void drawLog(glm::ivec2 const& fbSize, s32 logHeight)
{
	static s32 const s_yPad = 3;
	if (logHeight - s_yPad <= g_minDim.y)
	{
		return;
	}

	static s32 s_logLevel = 0;
	static char szFilter[64] = {0};
	bool bClear = false;
	std::string_view logFilter;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)fbSize.x, (f32)(logHeight - s_yPad)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(0.0f, (f32)(fbSize.y - logHeight + s_yPad)), ImGuiCond_Always);
	if (ImGui::Begin("Log", nullptr, flags))
	{
		// Widgets
		{
			ImGui::SameLine();
			bClear = ImGui::Button("Clear");
			ImGui::SameLine();
			ImGui::Checkbox("Auto-scroll", &g_bAutoScroll);
		}
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("Filter", szFilter, arraySize(szFilter));
			logFilter = szFilter;
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
			constexpr static s32 s_minCounter = 1, s_maxCounter = 20;
			s32 logEntries = (s32)g_maxLogEntries / 100;
			ImGui::PushButtonRepeat(true);
			if (ImGui::ArrowButton("##left", ImGuiDir_Left) && logEntries > s_minCounter)
			{
				--logEntries;
			}
			ImGui::SameLine(0.0f, spacing);
			if (ImGui::ArrowButton("##right", ImGuiDir_Right) && logEntries < s_maxCounter)
			{
				++logEntries;
			}
			ImGui::PopButtonRepeat();
			ImGui::SameLine();
			ImGui::Text("%d", logEntries * 100);
			g_maxLogEntries = (std::size_t)logEntries * 100;
		}
		{
			ImGui::Separator();
			ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
			if (bClear)
			{
				clearLog();
			}
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			auto lock = g_logMutex.lock();
			while (g_logEntries.size() > g_maxLogEntries)
			{
				g_logEntries.pop_front();
			}
			for (auto const& entry : g_logEntries)
			{
				if ((s32)entry.level >= s_logLevel && (logFilter.empty() || entry.text.find(logFilter) != std::string::npos))
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
	}
	ImGui::End();
}
} // namespace

bool editor::init(WindowID editorWindow)
{
	if (!g_bInit && gfx::ext_gui::isInit())
	{
		g_onLogChain = io::g_onLog;
		io::g_onLog = &guiLog;
		g_data.window = editorWindow;
		g_data.inputToken = Window::registerInput(
			[](Key key, Action action, Mods::VALUE mods) {
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
		g_editorCam.init(true);
		return g_bInit = true;
	}
	return false;
}

void editor::deinit()
{
	if (g_bInit)
	{
		io::g_onLog = g_onLogChain;
		g_onLogChain = nullptr;
		g_bInit = false;
		g_data = {};
		g_editorCam = {};
	}
	return;
}

void editor::tick([[maybe_unused]] Time dt)
{
	g_editorCam.m_state.flags[FreeCam::Flag::eEnabled] = !g_bTickGame;
	g_editorCam.tick(dt);
	static auto const smol = glm::vec2(0.6f);
	auto pWindow = WindowImpl::windowImpl(g_data.window);
	if (g_data.enabled && pWindow && pWindow->isOpen())
	{
		glm::vec2 centre = glm::vec2(0.5f * (f32)pWindow->windowSize().x, 0.0f);
		auto const fbSize = pWindow->framebufferSize();
		if (g_data.bAltPressed)
		{
			centre = pWindow->cursorPos();
		}
		g_gameRect = pWindow->m_pWindow->renderer().clampToView(centre, smol);
		if (gfx::ext_gui::isInit() && fbSize.x > 0 && fbSize.y > 0)
		{
			auto const logHeight = fbSize.y - (s32)(g_gameRect.bottom * (f32)fbSize.y);
			glm::ivec2 const leftPanelSize = {(s32)(g_gameRect.left * (f32)fbSize.x), fbSize.y - logHeight};
			glm::ivec2 const rightPanelSize = {fbSize.x - (s32)(g_gameRect.right * (f32)fbSize.x), fbSize.y - logHeight};
			auto pActive = World::active();
			if (!pActive || pActive != g_pWorld)
			{
				g_inspecting = {};
			}
			g_pWorld = pActive;
			drawLog(fbSize, logHeight);
			drawLeftPanel(fbSize, leftPanelSize);
			drawRightPanel(fbSize, rightPanelSize);
		}
	}
	else
	{
		g_gameRect = {};
	}
}
} // namespace le
#endif
