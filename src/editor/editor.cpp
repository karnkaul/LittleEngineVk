#include <deque>
#include <fstream>
#include <list>
#include <mutex>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <dumb_log/log.hpp>
#include <editor/editor.hpp>
#include <kt/async_queue/async_queue.hpp>
#if defined(LEVK_EDITOR)
#include <fmt/format.h>
#include <imgui.h>
#include <core/maths.hpp>
#include <core/utils.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/game/spring_arm.hpp>
#include <engine/gfx/render_driver.hpp>
#include <engine/gfx/viewport.hpp>
#include <engine/resources/resources.hpp>
#include <engine/window/input_types.hpp>
#include <engine/window/window.hpp>
#include <gfx/ext_gui.hpp>
#include <glm/gtc/quaternion.hpp>
#include <resources/model_impl.hpp>
#include <resources/resources_impl.hpp>
#include <window/window_impl.hpp>

namespace le::editor {
using namespace input;
using namespace std::literals;
using namespace ecs;

using v2 = glm::vec2 const&;
using iv2 = glm::ivec2 const&;
using sv = std::string_view;
template <typename T>
using il = std::initializer_list<T>;

struct Editor final {};
static std::string const s_tName = utils::tName<Editor>();

namespace {
bool g_bInit = false;
kt::lockable<std::mutex> g_logMutex;
dl::config::on_log::token g_token;

struct {
	WindowID window;
	OnInput::Tk inputToken;
	OnFocus::Tk focusToken;
	bool bEnabled = false;
	bool bFocus = true;
	std::unordered_set<input::Key> pressed;
	std::unordered_set<input::Key> held;
} g_data;

struct LogEntry final {
	std::string text;
	ImVec4 imColour;
	dl::level level;
};

std::size_t g_maxLogEntries = 500;
std::deque<LogEntry> g_logEntries;

glm::ivec2 const g_minDim = glm::ivec2(100, 100);

struct {
	Entity entity;
	Transform* pTransform = nullptr;
} g_inspecting;

struct {
	stdfs::path inspectID;
	std::string filter;
	bool bOpen = false;
} g_resources;

std::unordered_set<Entity> g_spawned;

void clicks(GUIState& out_state) {
	out_state[GUI::eLeftClicked] = ImGui::IsItemClicked(ImGuiMouseButton_Left);
	out_state[GUI::eRightClicked] = ImGui::IsItemClicked(ImGuiMouseButton_Right);
}

#pragma region log
bool g_bAutoScroll = true;

ImVec4 fromColour(Colour colour) {
	return ImVec4(colour.r.toF32(), colour.g.toF32(), colour.b.toF32(), colour.a.toF32());
}

Colour fromLevel(dl::level level) {
	switch (level) {
	case dl::level::error:
		return colours::red;
	case dl::level::warning:
		return colours::yellow;
	case dl::level::debug:
		return Colour(0x888888ff);
	default:
		return colours::white;
	}
}

void guiLog(sv text, dl::level level) {
	LogEntry entry;
	entry.text = std::string(text);
	entry.level = level;
	entry.imColour = fromColour(fromLevel(level));
	auto lock = g_logMutex.lock();
	g_logEntries.push_back(std::move(entry));
	while (g_logEntries.size() > g_maxLogEntries) {
		g_logEntries.pop_front();
	}
}

void clearLog() {
	g_logEntries.clear();
}
#pragma endregion log

#pragma region inspector
template <typename... Args>
bool dummy(Args...) {
	return true;
}

template <typename T, typename F1, typename F2>
void listResourceTree(typename io::PathTree<T>::Nodes nodes, F1 shouldList, F2 onSelected) {
	std::string_view filter = g_resources.filter.data();
	if (!filter.empty()) {
		auto removeNode = [filter](typename io::PathTree<T>::Node const& node) -> bool { return node.findPattern(filter, true) == nullptr; };
		nodes.erase(std::remove_if(nodes.begin(), nodes.end(), removeNode), nodes.end());
	}
	for (typename io::PathTree<T>::Node const& node : nodes) {
		auto const str = node.directory.filename().generic_string() + "/";
		if (auto n = TreeNode(str, false, false, true, true)) {
			for (auto const& [name, entry] : node.entries) {
				auto const& str = std::get<std::string>(entry);
				if (filter.empty() || str.find(filter) != std::string::npos) {
					auto const t = std::get<T>(entry);
					if (shouldList(t) && TreeNode(str, false, true, true, false).test(GUI::eLeftClicked)) {
						onSelected(t);
						g_resources.inspectID.clear();
					}
				}
			}
			listResourceTree<T, F1, F2>(node.childNodes(), shouldList, onSelected);
		}
	}
}

template <typename T, typename F1, typename F2>
void listResources(F1 shouldList, F2 onSelected, bool bNone = false) {
	static_assert(std::is_base_of_v<res::Resource<T>, T>, "T must derive from Resource");
	TWidget<std::string> f("Filter", g_resources.filter, 256.0f, 256);
	Styler s(Style::eSeparator);
	if (bNone) {
		auto const node = TreeNode("[None]", false, true, true, false);
		if (node.test(GUI::eLeftClicked)) {
			onSelected({});
			g_resources.inspectID.clear();
		}
	}
	listResourceTree<T>(res::loaded<T>().rootNodes(), shouldList, onSelected);
}

template <typename T>
void tabLoaded(sv tabName) {
	if (ImGui::BeginTabItem(tabName.data())) {
		listResources<T>(&dummy<T>, &dummy<T>);
		ImGui::EndTabItem();
	}
}

void resourcesWindow(v2 pos, v2 size) {
	static bool s_bResources = false;
	s_bResources |= static_cast<bool>(Button("Resources"));
	if (s_bResources) {
		ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Loaded Resources", &s_bResources, ImGuiWindowFlags_NoSavedSettings)) {
			if (ImGui::BeginTabBar("Resources")) {
				tabLoaded<res::Model>("Models");
				tabLoaded<res::Mesh>("Meshes");
				tabLoaded<res::Font>("Fonts");
				tabLoaded<res::Texture>("Textures");
				tabLoaded<res::Sampler>("Samplers");
				tabLoaded<res::Material>("Materials");
				tabLoaded<res::Shader>("Shaders");
				ImGui::EndTabBar();
			}
		}
		g_resources.bOpen |= s_bResources;
		ImGui::End();
	}
}

template <typename T, typename F, typename F2>
void inspectResource(T resource, sv selector, sv prefix, F onSelected, F2 filter, v2 pos, v2 size, bool bNone = true) {
	auto const id = res::info(resource).id;
	auto const resID = prefix / id;
	bool const bSelected = resID == g_resources.inspectID;
	auto node = TreeNode(id.empty() ? "[None]" : id.generic_string().data(), bSelected, true, true, false);
	if (node.test(GUI::eLeftClicked) || bSelected) {
		g_resources.inspectID = resID;
		ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_FirstUseEver);
		auto const label = fmt::format("{}###Resource Inspector", selector.empty() ? "[None]" : selector);
		bool bOpen = true;
		if (ImGui::Begin(label.data(), &bOpen, ImGuiWindowFlags_NoSavedSettings)) {
			listResources<T>(filter, onSelected, bNone);
		}
		g_resources.bOpen |= bOpen;
		ImGui::End();
		if (!bOpen) {
			g_resources.inspectID.clear();
		}
	}
}

void inspectMatInst(res::Mesh mesh, std::size_t idx, v2 pos, v2 size) {
	auto pInfo = res::infoRW(mesh);
	auto pImpl = res::impl(mesh);
	if (!pInfo || !pImpl) {
		return;
	}
	auto name = utils::tName<res::Material>();
	if (auto matInst = TreeNode(fmt::format("{} Instance {}", name, idx))) {
		constexpr std::array ids = {"Textured"sv, "Lit"sv, "Opaque"sv, "Drop Colour"sv};
		FlagsWidget<res::Material::Flags> material(ids, pInfo->material.flags);
		TWidget<Colour> tint("Tint", pInfo->material.tint);
		auto name = utils::tName<res::Texture>();
		if (auto diff = TreeNode(fmt::format("{}: Diffuse", name))) {
			inspectResource<res::Texture>(
				pInfo->material.diffuse, name, fmt::format("M{} diffuse", idx), [pInfo](res::Texture const& tex) { pInfo->material.diffuse = tex; },
				[](res::Texture const& tex) { return res::info(tex).type == res::Texture::Type::e2D; }, pos, size);
		}
		if (auto spec = TreeNode(fmt::format("{}: Specular", name))) {
			inspectResource<res::Texture>(
				pInfo->material.specular, name, fmt::format("M{} specular", idx), [pInfo](res::Texture const& tex) { pInfo->material.specular = tex; },
				[](res::Texture const& tex) { return res::info(tex).type == res::Texture::Type::e2D; }, pos, size);
		}
		name = utils::tName<res::Material>();
		if (auto mat = TreeNode(name)) {
			inspectResource<res::Material>(
				pInfo->material.material, name, fmt::format("M{} material", idx), [pInfo](res::Material const& mat) { pInfo->material.material = mat; },
				&dummy<res::Material>, pos, size, false);
			if (auto pMat = res::infoRW(mesh.material().material)) {
				if (auto albedo = TreeNode("Albedo")) {
					TWidget<Colour> ambient("Ka", pMat->albedo.ambient);
					TWidget<Colour> diffuse("Kd", pMat->albedo.diffuse);
					TWidget<Colour> specular("Ks", pMat->albedo.specular);
				}
				TWidget<f32> shininess("k", pMat->shininess);
			}
		}
	}
}

void entityInspector(v2 pos, v2 size, GameScene& out_scene) {
	Registry& registry = out_scene.m_registry;
	if (g_inspecting.entity != Entity()) {
		ImGui::LabelText("", "%s", registry.name(g_inspecting.entity).data());
		if (auto pInfo = registry.info(g_inspecting.entity)) {
			static std::string s_buf;
			TWidget<std::string> name("##EntityNameEdit", s_buf, 150.0f);
			sv const nn = s_buf.data();
			if (nn.empty()) {
				s_buf = pInfo->name;
			}
			if (Button("Edit")) {
				pInfo->name = s_buf.data();
				s_buf.clear();
			}
		}
		bool const bEnabled = registry.enabled(g_inspecting.entity);
		if (Button(bEnabled ? "Disable" : "Enable")) {
			registry.enable(g_inspecting.entity, !bEnabled);
		}
		Styler s(Style::eSameLine);
		if (auto search = g_spawned.find(g_inspecting.entity); search != g_spawned.end() && Button("Destroy")) {
			if (auto pTransform = registry.find<Transform>(g_inspecting.entity)) {
				Prop prop(g_inspecting.entity, *pTransform);
				out_scene.destroy(prop);
			} else {
				registry.destroy(g_inspecting.entity);
			}
			g_spawned.erase(g_inspecting.entity);
			g_inspecting = {};
		}
		s = Styler(Style::eSeparator);
		if (auto pDesc = registry.find<GameScene::Desc>(g_inspecting.entity)) {
			if (auto desc = TreeNode("gs::SceneDesc")) {
				if (auto skybox = TreeNode("Skybox")) {
					res::Texture tex;
					if (!pDesc->skyboxCubemapID.empty()) {
						tex = *res::find<res::Texture>(pDesc->skyboxCubemapID);
					}
					inspectResource(
						tex, "Skybox", "Skybox", [&pDesc](res::Texture tex) { pDesc->skyboxCubemapID = tex.info().id; },
						[](res::Texture tex) { return res::info(tex).type == res::Texture::Type::eCube; }, pos, size);
				}
				if (auto lights = TreeNode("Lights")) {
					s16 idx = 0;
					for (auto& dirLight : pDesc->dirLights) {
						if (auto node = TreeNode(fmt::format("dir_light_{}", idx++).data())) {
							if (node.test(GUI::eRightClicked)) {
								std::swap(dirLight, pDesc->dirLights.back());
								pDesc->dirLights.pop_back();
								idx = -1;
								break;
							}
							TWidget<glm::vec3> dir("dir", dirLight.direction, true);
							TWidget<Colour> ambient("ambient", dirLight.ambient);
							TWidget<Colour> diffuse("diffuse", dirLight.diffuse);
							TWidget<Colour> specular("specular", dirLight.specular);
						}
					}
					if (idx >= 0) {
						auto node = TreeNode("[Add]", false, true, false, true);
						if (node.test(GUI::eLeftClicked)) {
							pDesc->dirLights.push_back({});
						}
					}
				}
				if (auto camera = TreeNode("Camera")) {
					gfx::Camera& cam = pDesc->camera();
					TWidget<glm::vec3> pos("pos", cam.position, false);
					TWidget<glm::quat> orn("orn", cam.orientation);
				}
				TWidget<Colour> clearColour("Clear", pDesc->clearColour);
			}
		}
		if (g_inspecting.pTransform) {
			TWidget<Transform> t("Pos", "Orn", "Scl", *g_inspecting.pTransform);
			Styler s(Style::eSeparator);

			auto pMesh = registry.find<res::Mesh>(g_inspecting.entity);
			auto name = utils::tName<res::Mesh>();
			if (auto mesh = TInspector<res::Mesh>(registry, g_inspecting.entity, pMesh)) {
				inspectResource(
					*pMesh, name, name, [pMesh](res::Mesh mesh) { *pMesh = mesh; }, &dummy<res::Mesh>, pos, size);

				inspectMatInst(*pMesh, 0, pos, size);
			}
			auto pModel = registry.find<res::Model>(g_inspecting.entity);
			auto pModelImpl = pModel ? res::impl(*pModel) : nullptr;
			name = utils::tName<res::Model>();
			if (auto model = TInspector<res::Model>(registry, g_inspecting.entity, pModel)) {
				inspectResource(
					*pModel, name, name, [pModel](res::Model model) { *pModel = model; }, &dummy<res::Model>, pos, size);
				static std::deque<res::TScoped<res::Mesh>> s_empty;
				auto& meshes = pModelImpl ? pModelImpl->loadedMeshes() : s_empty;
				std::size_t idx = 0;
				for (auto& mesh : meshes) {
					inspectMatInst(mesh, idx++, pos, size);
				}
			}
			if (auto pSpring = registry.find<SpringArm>(g_inspecting.entity)) {
				if (auto i = TInspector<SpringArm>(gs::g_game.m_registry, g_inspecting.entity, pSpring, "Spring")) {
					static constexpr f32 w = 50.0f;
					TWidget<f32> k("k", pSpring->k, 0.01f, w);
					Styler s(Style::eSameLine);
					TWidget<f32> b("b", pSpring->b, 0.001f, w);
					TWidget<glm::vec3> o("offset", pSpring->offset, false);
				}
			}
		}
		if (!out_scene.m_editorData.inspect.empty()) {
			Styler s(Style::eSeparator);
			for (auto& f : out_scene.m_editorData.inspect) {
				f(g_inspecting.entity, g_inspecting.pTransform);
			}
		}
	}
}
#pragma endregion inspector

#pragma region widgets
void playButton() {
	char const* szPlayPause = g_bTickGame ? "Pause" : "Play";
	if (!g_bStepGame.t && Button(szPlayPause)) {
		g_bTickGame = !g_bTickGame;
		logI("[{}] {}", s_tName, g_bTickGame ? "Resumed" : "Paused");
	}
	if (!g_bTickGame) {
		Styler s(Style::eSameLine);
		static s64 step = 1;
		TWidget<std::pair<s64, s64>> st(fmt::format("{}", step), step, 1, 240, 1);
		if (!g_bStepGame.t) {
			ImGui::SameLine(0.0f, 3.0f);
			if (Button("Step")) {
				g_bStepGame = TTrigger<bool>(true, (s16)step);
			}
		}
	}
} // namespace

void presentModeDropdown() {
	if (auto pWindow = WindowImpl::windowImpl(g_data.window)) {
		static std::array const presentModeNames = {"Off"sv, "Mailbox"sv, "FIFO"sv, "FIFO (Relaxed)"sv};
		auto presentMode = pWindow->presentMode();
		auto const& presentModes = pWindow->m_presentModes;
		std::vector<std::string_view> list;
		list.reserve(presentModes.size());
		for (auto mode : presentModes) {
			list.push_back(presentModeNames[(std::size_t)mode]);
		}
		if (auto vsync = Combo("Vsync", list, presentModeNames[(std::size_t)presentMode]); vsync.select >= 0 && vsync.select < (s32)presentModes.size()) {
			auto newMode = presentModes[(std::size_t)vsync.select];
			if (newMode != presentMode) {
				if (pWindow->setPresentMode(newMode)) {
					logI("[{}] Switched present mode from [{}] to [{}]", s_tName, presentModeNames[(std::size_t)presentMode],
						 presentModeNames[(std::size_t)newMode]);
				} else {
					logE("[{}] Failed to switch present mode!", s_tName);
				}
			}
		}
	}
}

void logLevelDropdown() {
	if (auto level = Combo("Log Level", dl::config::g_log_levels, dl::config::g_log_levels[(std::size_t)dl::config::g_min_level]); level.select >= 0) {
		if (level.select != (s32)dl::config::g_min_level) {
			auto const oldLevel = dl::config::g_log_levels[(std::size_t)dl::config::g_min_level];
			dl::config::g_min_level = dl::level::debug;
			logI("[{}] Min Log Level changed from [{}] to [{}]", s_tName, oldLevel, dl::config::g_log_levels[(std::size_t)level.select]);
			dl::config::g_min_level = (dl::level)level.select;
		}
	}
}
#pragma endregion widgets

#pragma region sceneIO
namespace sceneIO {
std::string camelify(std::string_view str) {
	std::string ret;
	ret.reserve(str.size());
	for (auto c : str) {
		if (std::isupper(c) && !ret.empty()) {
			ret += '_';
		}
		ret += (char)std::tolower(c);
	}
	return ret;
}

void addMesh(dj::object& out_entry, res::Mesh mesh) {
	out_entry.add<dj::string>("mesh_id", mesh.info().id.generic_string());
}

void addModel(dj::object& out_entry, res::Model model) {
	out_entry.add<dj::string>("model_id", model.info().id.generic_string());
}

void addDesc(dj::object& out_entry, GameScene::Desc const& desc) {
	dj::object d;
	if (!desc.skyboxCubemapID.empty()) {
		d.add<dj::string>("skybox_id", desc.skyboxCubemapID.generic_string());
	}
	d.add<dj::string>("clear_colour", desc.clearColour.toStr(true));
	out_entry.add("scene_desc", std::move(d));
}

void walkSceneTree(dj::array& out_root, Transform& root, GameScene::EntityMap const& emap, Registry const& reg, std::unordered_set<Entity>& out_added) {
	auto const children = root.children();
	auto search = emap.find(root);
	if (search != emap.end()) {
		auto [transform, entity] = *search;
		out_added.insert(entity);
		dj::object e;
		e.add<dj::string>("name", reg.name(entity));
		if (auto pMesh = reg.find<res::Mesh>(entity)) {
			addMesh(e, *pMesh);
		}
		if (auto pModel = reg.find<res::Model>(entity)) {
			addModel(e, *pModel);
		}
		Transform& t = transform;
		auto const p = t.position();
		auto pos = fmt::format("{} \"x\": {}, \"y\": {}, \"z\": {} {}", '{', p.x, p.y, p.z, '}');
		auto const s = t.scale();
		auto scl = fmt::format("{} \"x\": {}, \"y\": {}, \"z\": {} {}", '{', s.x, s.y, s.z, '}');
		auto const o = t.orientation();
		auto orn = fmt::format("{} \"x\": {}, \"y\": {}, \"z\": {}, \"w\": {} {}", '{', o.x, o.y, o.z, o.w, '}');
		dj::object tr;
		tr.add<dj::object>("position", std::move(pos));
		tr.add<dj::object>("orientation", std::move(orn));
		tr.add<dj::object>("scale", std::move(scl));
		auto const& children = t.children();
		if (!children.empty()) {
			dj::array arr;
			for (Transform& child : children) {
				walkSceneTree(arr, child, emap, reg, out_added);
			}
			tr.add("children", std::move(arr));
		}
		e.add("transform", std::move(tr));
		out_root.add(std::move(e));
	}
}

void residue(dj::array& out_arr, Registry const& reg, std::unordered_set<Entity> const& added) {
	{
		auto view = reg.view<GameScene::Desc>();
		for (auto& [entity, query] : view) {
			if (added.find(entity) == added.end()) {
				dj::object e;
				auto const& [desc] = query;
				e.add<dj::string>("name", reg.name(entity));
				addDesc(e, desc);
				out_arr.add(std::move(e));
			}
		}
	}
}

dj::object serialise(GameScene const& scene) {
	dj::object ret;
	dj::array entities;
	std::unordered_set<Entity> added;
	for (auto& child : scene.m_sceneRoot.children()) {
		walkSceneTree(entities, child, scene.m_entityMap, scene.m_registry, added);
	}
	residue(entities, scene.m_registry, added);
	ret.add("entities", std::move(entities));
	return ret;
}
} // namespace sceneIO
#pragma endregion sceneIO

#pragma region layout
void drawLeftPanel([[maybe_unused]] iv2 fbSize, iv2 panelSize, GameScene& out_scene) {
	if (panelSize.x < g_minDim.x || panelSize.y < g_minDim.y) {
		return;
	}
	static s32 const s_xPad = 2;
	static s32 const s_dy = 2;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)(panelSize.x - s_xPad - s_xPad), (f32)(panelSize.y - s_dy)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(s_xPad, (f32)s_dy), ImGuiCond_Always);
	if (ImGui::Begin("Inspector", nullptr, flags)) {
		resourcesWindow({(f32)(panelSize.x + s_xPad), 200.0f}, {500.0f, 300.0f});
		static bool s_bImGuiDemo = false;
		s_bImGuiDemo |= static_cast<bool>(Button("ImGui Demo"));
		if (s_bImGuiDemo) {
			ImGui::ShowDemoWindow(&s_bImGuiDemo);
		}
		Styler s(Style::eSeparator);
		if (Button("test save")) {
			auto const filename = fmt::format("{}.scene", sceneIO::camelify(gs::g_game.m_name));
			if (auto file = std::ofstream(filename)) {
				auto const scene = sceneIO::serialise(out_scene);
				auto opts = dj::g_serial_opts;
				opts.sort_keys = opts.pretty = true;
				auto const str = scene.serialise(opts);
				if (file.write(str.data(), (std::streamsize)str.size())) {
					logI("[{}] Scene saved to [{}]", s_tName, filename);
				} else {
					logE("[{}] Failed to save scene to [{}]!", s_tName, filename);
				}
			} else {
				logE("[{}] Failed to open [{}] to save scene!", s_tName, filename);
			}
		}
		entityInspector({(f32)(panelSize.x + s_xPad * 4), 200.0f}, {400.0f, 300.0f}, out_scene);
	}
	ImGui::End();
	return;
}

void drawRightPanel([[maybe_unused]] iv2 fbSize, iv2 panelSize, GameScene& out_scene) {
	if (panelSize.x < g_minDim.x || panelSize.y < g_minDim.y) {
		return;
	}
	static s32 const s_xPad = 2;
	static s32 const s_dy = 2;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2((f32)(panelSize.x - s_xPad - s_xPad), (f32)(panelSize.y - s_dy)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2((f32)(fbSize.x - panelSize.x + s_xPad), (f32)s_dy), ImGuiCond_Always);
	if (ImGui::Begin("Scene", nullptr, flags)) {
		playButton();
		if (ImGui::BeginTabBar("Right")) {
			if (ImGui::BeginTabItem("Scene")) {
				Registry& registry = out_scene.m_registry;
				{
					static std::string s_buf;
					TWidget<std::string>("Name", s_buf, 150.0f);
					sv const nn = s_buf.data();
					if (nn.empty()) {
						s_buf = "[Unnamed]";
					}
					if (Button("Spawn Prop")) {
						auto prop = out_scene.spawnProp(nn.data());
						g_inspecting = {prop.entity, prop.pTransform};
						g_spawned.insert(g_inspecting.entity);
						s_buf.clear();
					}
				}
				auto s = Styler(Style::eSeparator);
				auto view = registry.view<GameScene::Desc>();
				if (!view.empty()) {
					auto [entity, _] = *view.begin();
					auto const node = TreeNode(registry.name(entity), g_inspecting.entity == entity, true, true, false);
					if (node.test(GUI::eLeftClicked)) {
						bool const bSelect = g_inspecting.entity != entity;
						g_inspecting = {bSelect ? entity : Entity(), nullptr};
					}
				}
				s = Styler(Style::eSeparator);
				Transform::walk(out_scene.m_sceneRoot, [&](Transform& t) -> bool {
					auto search = out_scene.m_entityMap.find(t);
					if (search != out_scene.m_entityMap.end()) {
						auto [transform, entity] = *search;
						Transform& t = transform;
						auto node = TreeNode(registry.name(entity), g_inspecting.entity == entity, t.children().empty(), true, false);
						if (node.test(GUI::eLeftClicked)) {
							bool const bSelect = g_inspecting.entity != entity;
							g_inspecting = {bSelect ? entity : Entity(), bSelect ? &t : nullptr};
						}
						return (node.test(GUI::eOpen));
					}
					return false;
				});
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Options")) {
				presentModeDropdown();
				Styler s(Style::eSeparator);
				logLevelDropdown();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Custom")) {
				if (!g_bStepGame.t && !engine::busy()) {
					for (auto& f : out_scene.m_editorData.customRightPanel) {
						f();
					}
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
	return;
}

void drawLog(iv2 fbSize, s32 logHeight) {
	static s32 const s_yPad = 3;
	if (logHeight - s_yPad <= g_minDim.y) {
		return;
	}

	static Time elapsed = Time::elapsed();
	Time frameTime = Time::elapsed() - elapsed;
	elapsed = Time::elapsed();
	static constexpr std::size_t maxFTs = 200;
	static std::array<f32, maxFTs> ftArr;
	static std::list<Time> fts;
	fts.push_back(frameTime);
	while (fts.size() > maxFTs) {
		fts.pop_front();
	}
	std::size_t idx = 0;
	for (auto ft : fts) {
		ftArr[idx++] = ((f32)ft.to_us() * 0.001f);
	}
	for (; idx < ftArr.size(); ++idx) {
		ftArr[idx] = 0.0f;
	}

	static s32 s_logLevel = 0;
	static char szFilter[64] = {0};
	bool bClear = false;
	sv logFilter;

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowSize(ImVec2((f32)fbSize.x, (f32)(logHeight - s_yPad)), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(0.0f, (f32)(fbSize.y - logHeight + s_yPad)), ImGuiCond_Always);
	if (ImGui::Begin("##Log", nullptr, flags)) {
		// Frame time
		{
			auto str = fmt::format("{:.3}ms", frameTime.to_s() * 1000);
			ImGui::PlotLines("Frame Time", ftArr.data(), (s32)ftArr.size(), 0, str.data());
			Styler s(Style::eSeparator);
		}
		// TWidgets
		{
			bClear = static_cast<bool>(Button("Clear"));
			Styler s(Style::eSameLine);
			ImGui::Checkbox("Auto-scroll", &g_bAutoScroll);
		}
		{
			Styler s(Style::eSameLine);
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("Filter", szFilter, arraySize(szFilter));
			logFilter = szFilter;
		}
		{
			Styler s(Style::eSameLine);
			ImGui::RadioButton("All", &s_logLevel, 0);
			s = Styler(Style::eSameLine);
			ImGui::RadioButton("Info", &s_logLevel, 1);
			s = Styler(Style::eSameLine);
			ImGui::RadioButton("Warning", &s_logLevel, 2);
			s = Styler(Style::eSameLine);
			ImGui::RadioButton("Error", &s_logLevel, 3);
		}
		{
			Styler s(Style::eSameLine);
			static constexpr s64 s_minTCounter = 1, s_maxTCounter = 20;
			s64 logEntries = (s64)g_maxLogEntries / 100;
			TWidget<std::pair<s64, s64>> st(fmt::format("{}", logEntries * 100), logEntries, s_minTCounter, s_maxTCounter, 1);
			g_maxLogEntries = (std::size_t)logEntries * 100;
		}
		{
			Styler s(Style::eSeparator);
			ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
			if (bClear) {
				clearLog();
			}
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			auto lock = g_logMutex.lock();
			while (g_logEntries.size() > g_maxLogEntries) {
				g_logEntries.pop_front();
			}
			for (auto const& entry : g_logEntries) {
				if ((s32)entry.level >= s_logLevel && (logFilter.empty() || entry.text.find(logFilter) != std::string::npos)) {
					ImGui::TextColored(entry.imColour, "%s", entry.text.data());
				}
			}
			ImGui::PopStyleVar();
			if (g_bAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
				ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();
}

void resize(WindowImpl& out_w, gfx::Viewport& out_vp) {
	enum class Handle : std::size_t { eNone, eLeft, eRight, eBottom, eLeftBottom, eRightBottom };
	static constexpr f32 nDelta = 0.01f;
	static constexpr glm::vec2 scaleLimit = {0.33f, 0.98f};
	static constexpr f32 xPad = 1.0f - scaleLimit.y - 0.015f;
	static Handle s_resizing = Handle::eNone;
	static gfx::Viewport s_prevVP = out_vp;
	if (g_resources.bOpen) {
		return;
	}
	auto const wSize = out_w.windowSize();
	auto const cursorPos = input::cursorPosition(true);
	glm::vec2 const nCursorPos = {cursorPos.x / (f32)wSize.x, cursorPos.y / (f32)wSize.y};
	bool const bClickPressed = g_data.pressed.find(Key::eMouseButton1) != g_data.pressed.end();
	bool const bClickHeld = g_data.held.find(Key::eMouseButton1) != g_data.held.end();
	input::CursorType toSet = input::CursorType::eDefault;
	auto endResize = [&toSet]() {
		s_resizing = Handle::eNone;
		toSet = input::CursorType::eDefault;
		ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
	};
	auto checkResize = [bClickPressed, &toSet, &out_vp](f32 s0, f32 t0, f32 s1, f32 t1, input::CursorType c, Handle h) {
		if (maths::equals(s0, t0, nDelta) && maths::equals(s1, t1, nDelta)) {
			toSet = c;
			if (bClickPressed) {
				s_prevVP = out_vp;
				s_resizing = h;
			}
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		}
	};
	if (!bClickHeld) {
		endResize();
	}
	if (g_data.pressed.find(Key::eEscape) != g_data.pressed.end()) {
		out_vp = s_prevVP;
		endResize();
	} else {
		auto clampTL = [](f32 tl, f32 s) -> f32 { return std::clamp(tl, xPad, 1.0f - s - xPad); };
		switch (s_resizing) {
		case Handle::eNone: {
			auto const rect = out_vp.rect();
			checkResize(nCursorPos.x, rect.lt.x, 0.0f, 0.0f, input::CursorType::eResizeEW, Handle::eLeft);
			checkResize(nCursorPos.x, rect.rb.x, 0.0f, 0.0f, input::CursorType::eResizeEW, Handle::eRight);
			checkResize(nCursorPos.y, rect.rb.y, 0.0f, 0.0f, input::CursorType::eResizeNS, Handle::eBottom);
			checkResize(nCursorPos.x, rect.lt.x, nCursorPos.y, rect.rb.y, input::CursorType::eResizeNESW, Handle::eLeftBottom);
			checkResize(nCursorPos.x, rect.rb.x, nCursorPos.y, rect.rb.y, input::CursorType::eResizeNWSE, Handle::eRightBottom);
			break;
		}
		case Handle::eLeft: {
			toSet = input::CursorType::eResizeEW;
			out_vp.topLeft.x = clampTL(nCursorPos.x, out_vp.scale);
			break;
		}
		case Handle::eRight: {
			toSet = input::CursorType::eResizeEW;
			out_vp.topLeft.x = clampTL(nCursorPos.x - out_vp.scale, out_vp.scale);
			break;
		}
		case Handle::eBottom: {
			toSet = input::CursorType::eResizeNS;
			auto const centre = out_vp.topLeft.x + out_vp.scale * 0.5f;
			f32 const s = out_vp.clampScale(nCursorPos.y, scaleLimit);
			out_vp.topLeft.x = clampTL(centre - s * 0.5f, s);
			break;
		}
		case Handle::eLeftBottom: {
			toSet = input::CursorType::eResizeNESW;
			out_vp.topLeft.x = clampTL(nCursorPos.x, out_vp.clampScale(nCursorPos.y, scaleLimit));
			break;
		}
		case Handle::eRightBottom: {
			toSet = input::CursorType::eResizeNWSE;
			f32 const s = out_vp.clampScale(nCursorPos.y, scaleLimit);
			out_vp.topLeft.x = clampTL(nCursorPos.x - s, s);
			break;
		}
		default:
			break;
		}
	}
	out_w.setCursorType(toSet);
}
#pragma endregion layout
} // namespace

Styler::Styler(StyleFlags flags) {
	if (flags.test(Style::eSameLine)) {
		ImGui::SameLine();
	}
	if (flags.test(Style::eSeparator)) {
		ImGui::Separator();
	}
}

GUIStateful::GUIStateful() {
	clicks(guiState);
}

GUIStateful::GUIStateful(GUIStateful&& rhs) : guiState(std::exchange(rhs.guiState, GUIState{})) {
}

GUIStateful& GUIStateful::operator=(GUIStateful&& rhs) {
	if (&rhs != this) {
		guiState = std::exchange(rhs.guiState, GUIState{});
	}
	return *this;
}

void GUIStateful::refresh() {
	clicks(guiState);
}

Text::Text(sv text) {
	ImGui::Text("%s", text.data());
}

Button::Button(sv id) {
	refresh();
	guiState[GUI::eLeftClicked] = ImGui::Button(id.empty() ? "[Unnamed]" : id.data());
}

Combo::Combo(sv id, Span<sv> entries, sv preSelected) {
	if (!entries.empty()) {
		guiState[GUI::eOpen] = ImGui::BeginCombo(id.empty() ? "[Unnamed]" : id.data(), preSelected.data());
		refresh();
		if (test(GUI::eOpen)) {
			std::size_t i = 0;
			for (auto entry : entries) {
				bool const bSelected = preSelected == entry;
				if (ImGui::Selectable(entry.data(), bSelected)) {
					select = (s32)i;
					selected = entry;
				}
				if (bSelected) {
					ImGui::SetItemDefaultFocus();
				}
				++i;
			}
			ImGui::EndCombo();
		}
	}
}

TreeNode::TreeNode() = default;

TreeNode::TreeNode(sv id) {
	guiState[GUI::eOpen] = ImGui::TreeNode(id.empty() ? "[Unnamed]" : id.data());
	refresh();
}

TreeNode::TreeNode(sv id, bool bSelected, bool bLeaf, bool bFullWidth, bool bLeftClickOpen) {
	static constexpr ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	ImGuiTreeNodeFlags const branchFlags = (bLeftClickOpen ? 0 : ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick);
	ImGuiTreeNodeFlags const metaFlags = (bSelected ? ImGuiTreeNodeFlags_Selected : 0) | (bFullWidth ? ImGuiTreeNodeFlags_SpanAvailWidth : 0);
	ImGuiTreeNodeFlags const nodeFlags = (bLeaf ? leafFlags : branchFlags) | metaFlags;
	guiState[GUI::eOpen] = ImGui::TreeNodeEx(id.empty() ? "[Unnamed]" : id.data(), nodeFlags) && !bLeaf;
	refresh();
}

TreeNode::~TreeNode() {
	if (test(GUI::eOpen)) {
		ImGui::TreePop();
	}
}

TWidget<bool>::TWidget(sv id, bool& out_b) {
	ImGui::Checkbox(id.empty() ? "[Unnamed]" : id.data(), &out_b);
}

TWidget<s32>::TWidget(sv id, s32& out_s, f32 w) {
	if (w > 0.0f) {
		ImGui::SetNextItemWidth(w);
	}
	ImGui::DragInt(id.empty() ? "[Unnamed]" : id.data(), &out_s);
}

TWidget<f32>::TWidget(sv id, f32& out_f, f32 df, f32 w) {
	if (w > 0.0f) {
		ImGui::SetNextItemWidth(w);
	}
	ImGui::DragFloat(id.empty() ? "[Unnamed]" : id.data(), &out_f, df);
}

TWidget<Colour>::TWidget(sv id, Colour& out_colour) {
	auto c = out_colour.toVec4();
	ImGui::ColorEdit3(id.empty() ? "[Unnamed]" : id.data(), &c.x);
	out_colour = Colour(c);
}

TWidget<std::string>::TWidget(sv id, ZeroedBuf& out_buf, f32 width, std::size_t max) {
	if (max <= (std::size_t)width) {
		max = (std::size_t)width;
	}
	out_buf.reserve(max);
	if (out_buf.size() < max) {
		std::size_t const diff = max - out_buf.size();
		std::string str(diff, '\0');
		out_buf += str;
	}
	ImGui::SetNextItemWidth(width);
	ImGui::InputText(id.empty() ? "[Unnamed]" : id.data(), out_buf.data(), max);
}

TWidget<glm::vec2>::TWidget(sv id, glm::vec2& out_vec, bool bNormalised, f32 dv) {
	if (bNormalised) {
		if (glm::length2(out_vec) <= 0.0f) {
			out_vec = gfx::g_nFront;
		} else {
			out_vec = glm::normalize(out_vec);
		}
	}
	ImGui::DragFloat2(id.empty() ? "[Unnamed]" : id.data(), &out_vec.x, dv);
	if (bNormalised) {
		out_vec = glm::normalize(out_vec);
	}
}

TWidget<glm::vec3>::TWidget(sv id, glm::vec3& out_vec, bool bNormalised, f32 dv) {
	if (bNormalised) {
		if (glm::length2(out_vec) <= 0.0f) {
			out_vec = gfx::g_nFront;
		} else {
			out_vec = glm::normalize(out_vec);
		}
	}
	ImGui::DragFloat3(id.empty() ? "[Unnamed]" : id.data(), &out_vec.x, dv);
	if (bNormalised) {
		out_vec = glm::normalize(out_vec);
	}
}

TWidget<glm::quat>::TWidget(sv id, glm::quat& out_quat, f32 dq) {
	auto rot = glm::eulerAngles(out_quat);
	ImGui::DragFloat3(id.empty() ? "[Unnamed]" : id.data(), &rot.x, dq);
	out_quat = glm::quat(rot);
}

TWidget<Transform>::TWidget(sv idPos, sv idOrn, sv idScl, Transform& out_t, glm::vec3 const& dPOS) {
	auto posn = out_t.position();
	auto scl = out_t.scale();
	auto const& orn = out_t.orientation();
	auto rot = glm::eulerAngles(orn);
	ImGui::DragFloat3(idPos.data(), &posn.x, dPOS.x);
	out_t.position(posn);
	ImGui::DragFloat3(idOrn.data(), &rot.x, dPOS.y);
	out_t.orient(glm::quat(rot));
	ImGui::DragFloat3(idScl.data(), &scl.x, dPOS.z);
	out_t.scale(scl);
}

TWidget<std::pair<s64, s64>>::TWidget(sv id, s64& out_t, s64 min, s64 max, s64 dt) {
	ImGui::PushButtonRepeat(true);
	if (ImGui::ArrowButton("##left", ImGuiDir_Left) && out_t > min) {
		out_t -= dt;
	}
	ImGui::SameLine(0.0f, 3.0f);
	if (ImGui::ArrowButton("##right", ImGuiDir_Right) && out_t < max) {
		out_t += dt;
	}
	ImGui::PopButtonRepeat();
	ImGui::SameLine(0.0f, 5.0f);
	ImGui::Text("%s", id.data());
}
} // namespace le::editor

namespace le {
bool editor::init(WindowID editorWindow) {
	if (!g_bInit && gfx::ext_gui::isInit()) {
		g_token = dl::config::g_on_log.add(&guiLog);
		g_data.window = editorWindow;
		g_data.inputToken = Window::registerInput(
			[](Key key, Action action, Mods::VALUE mods) {
				if (key == Key::eE && action == Action::eRelease && mods & Mods::eCONTROL) {
					g_data.bEnabled = !g_data.bEnabled;
				}
				switch (action) {
				case Action::ePress:
					g_data.held.insert(key);
					g_data.pressed.insert(key);
					break;
				case Action::eRelease:
					g_data.held.erase(key);
					break;
				default:
					break;
				}
			},
			editorWindow);
		if (auto pWindow = WindowImpl::windowImpl(g_data.window)) {
			g_data.focusToken = pWindow->m_pWindow->registerFocus([](bool bFocus) { g_data.bFocus = bFocus; });
		}
		g_editorCam.init(true);
		return g_bInit = true;
	}
	return false;
}

bool editor::enabled() {
	return g_data.bEnabled;
}

void editor::deinit() {
	if (g_bInit) {
		g_token = {};
		g_bInit = false;
		g_data = {};
		g_editorCam = {};
	}
	return;
}

std::optional<gfx::Viewport> editor::tick(GameScene& out_scene, Time dt) {
	static gfx::Viewport s_comboView = {{0.2f, 0.0f}, 0.6f};
	g_editorCam.m_state.flags[FreeCam::Flag::eEnabled] = !g_bTickGame;
	g_editorCam.tick(dt);
	auto pWindow = WindowImpl::windowImpl(g_data.window);
	if (g_data.bEnabled && pWindow && pWindow->open()) {
		auto const fbSize = pWindow->framebufferSize();
		resize(*pWindow, s_comboView);
		auto const rect = s_comboView.rect();
		g_resources.bOpen = false;
		if (gfx::ext_gui::isInit() && fbSize.x > 0 && fbSize.y > 0) {
			auto const logHeight = fbSize.y - (s32)(rect.rb.y * (f32)fbSize.y);
			glm::ivec2 const leftPanelSize = {(s32)(rect.lt.x * (f32)fbSize.x), fbSize.y - logHeight};
			glm::ivec2 const rightPanelSize = {fbSize.x - (s32)(rect.rb.x * (f32)fbSize.x), fbSize.y - logHeight};
			Registry& reg = out_scene.m_registry;
			if (!reg.exists(g_inspecting.entity)) {
				g_inspecting = {};
			}
			drawLog(fbSize, logHeight);
			drawLeftPanel(fbSize, leftPanelSize, out_scene);
			drawRightPanel(fbSize, rightPanelSize, out_scene);
		}
	}
	g_data.pressed.clear();
	out_scene.m_editorData = {};
	if (g_data.bEnabled) {
		return s_comboView;
	}
	return std::nullopt;
}
} // namespace le
#endif
