#include <editor/levk_imgui.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/gameplay/editor/editor.hpp>
#include <levk/gameplay/editor/types.hpp>

#if defined(LEVK_EDITOR)
#include <editor/sudo.hpp>
#include <levk/engine/assets/asset_provider.hpp>
#include <levk/engine/render/pipeline.hpp>
#include <levk/gameplay/editor/asset_index.hpp>
#include <levk/gameplay/editor/inspector.hpp>
#include <levk/gameplay/editor/log_stats.hpp>
#include <levk/gameplay/editor/main_menu.hpp>
#include <levk/gameplay/editor/palette_tab.hpp>
#include <levk/gameplay/editor/palettes/settings.hpp>
#include <levk/gameplay/editor/resizer.hpp>
#include <levk/gameplay/editor/scene_tree.hpp>
#include <levk/graphics/render/renderer.hpp>
#include <levk/graphics/skybox.hpp>
#endif

namespace le::editor {
namespace {
enum class Flag { eEngaged, eRenderReady, eShowDemo };
using Flags = ktl::enum_flags<Flag, u8>;

#if defined(LEVK_EDITOR)
template <typename T>
class EditorTab : public PaletteTab {
	void loopItems(SceneRef scene) override {
		if (auto it = TabBar::Item(T::title_v)) { Sudo::update<T>(scene); }
		PaletteTab::loopItems(scene);
	}
};

void displayScale([[maybe_unused]] f32 renderScale) {
#if defined(LEVK_USE_IMGUI)
	auto& ds = ImGui::GetIO().DisplayFramebufferScale;
	ds = {ds.x * renderScale, ds.y * renderScale};
#endif
}

void inspectSkyboxP(Inspect<AssetProvider<graphics::Skybox>> provider) {
	Text uri(provider.store.uri<graphics::Skybox>(provider.get().uri()));
	if (auto popup = Popup("Skybox##inspect_skybox_provider")) {
		static ktl::stack_string<128> s_search;
		TWidget<char*> search("Search##inspect_skybox_provider", s_search.c_str(), s_search.capacity());
		if (auto select = AssetIndex::list<graphics::Skybox>(s_search)) {
			provider.get().uri(select.item);
			popup.close();
		}
	}
	if (Button("Edit...")) { Popup::open("Skybox##inspect_skybox_provider"); }
}

void inspectRLP(Inspect<RenderPipeProvider> provider) {
	Text uri(provider.store.uri<RenderPipeline>(provider.get().uri()));
	if (auto popup = Popup("RenderPipeline##inspect_pipe_provider")) {
		static ktl::stack_string<128> s_search;
		TWidget<char*> search("Search##inspect_pipe_provider", s_search.c_str(), s_search.capacity());
		if (auto select = AssetIndex::list<RenderPipeline>(s_search)) {
			provider.get().uri(select.item);
			popup.close();
		}
	}
	if (Button("Edit...")) { Popup::open("RenderPipeline##inspect_pipe_provider"); }
}
#endif

struct Rail {
	std::string_view id;
#if defined(LEVK_EDITOR)
	std::unique_ptr<PaletteTab> tab;
#endif
};

struct Storage {
#if defined(LEVK_EDITOR)
	Resizer resizer;
	LogStats logStats;
	MainMenu mainMenu;
	MenuList menu;

	Rail left = {"Left", {}};
	Rail right = {"Right", {}};

#endif
};

struct Cache {
#if defined(LEVK_EDITOR)
	Inspecting inspect;
#endif
	dens::registry const* prev{};
};

struct State {
	Opt<Storage> storage{};
	Viewport gameView{};
	Flags flags;
	Cache cache;
};

State g_state;
} // namespace

struct Instance::Impl {
	inline static Opt<Impl> s_impl{};

	std::optional<DearImGui> imgui;
	Storage storage;
	Engine::Signal rendererChanged;
	Engine::Service engine;

	Impl(Engine::Service engine) noexcept : engine(engine) {
		s_impl = this;
		g_state.storage = &storage;
	}

	~Impl() noexcept {
		s_impl = {};
		g_state.storage = {};
		g_state.flags.reset(Flag::eRenderReady);
#if defined(LEVK_EDITOR)
		Sudo::clear<Inspector, SceneTree>();
#endif
	}
};

Instance::Instance() noexcept = default;
Instance::Instance(std::unique_ptr<Impl>&& impl) noexcept : m_impl(std::move(impl)) {}
Instance::Instance(Instance&&) noexcept = default;
Instance& Instance::operator=(Instance&&) noexcept = default;
Instance::~Instance() noexcept = default;

Instance::operator bool() const noexcept { return m_impl != nullptr; }

Instance Instance::make(Engine::Service engine) {
	EXPECT(engine.booted());
	if (!engine.booted()) {
		logE(LC_LibUser, "[Editor] Cannot make Instance using unbooted engine!");
		return {};
	}
	if (Instance::Impl::s_impl) {
		logW(LC_LibUser, "[Editor] make(): Instance already exists");
		return {};
	}
	DearImGui imgui;
	if (!imgui.init(engine.context(), engine.window())) {
		logE(LC_LibUser, "[Editor] Failed to initialize Dear ImGui!");
		return {};
	}
	auto impl = std::make_unique<Instance::Impl>(engine);
	impl->imgui.emplace(std::move(imgui));
	impl->rendererChanged = engine.onRendererChanged();
	impl->rendererChanged += [inst = impl.get(), engine] { inst->imgui->init(engine.context(), engine.window()); };
#if defined(LEVK_EDITOR)
	impl->storage.left.tab = std::make_unique<EditorTab<SceneTree>>();
	impl->storage.left.tab->attach<Settings>("Settings");
	impl->storage.right.tab = std::make_unique<EditorTab<Inspector>>();
	Inspector::attach<RenderPipeProvider>(&inspectRLP, {}, "RenderPipeline");
	Inspector::attach<AssetProvider<graphics::Skybox>>(&inspectSkyboxP, {}, "Skybox");
#endif
	g_state.gameView = g_comboView;
	return Instance(std::move(impl));
}

MenuList& menu() noexcept {
	static MenuList s_menu{};
#if defined(LEVK_EDITOR)
	return g_state.storage ? g_state.storage->menu : s_menu;
#else
	return s_menu;
#endif
}

Viewport const& view() noexcept {
	static constexpr Viewport fallack{};
	return g_state.flags.test(Flag::eEngaged) ? g_state.gameView : fallack;
}

bool exists() noexcept { return Instance::Impl::s_impl != nullptr; }
void showImGuiDemo() noexcept { g_state.flags.set(Flag::eShowDemo); }

bool engaged() noexcept { return g_state.flags.test(Flag::eEngaged); }
void engage(bool set) noexcept { g_state.flags.assign(Flag::eEngaged, set); }
void toggle() noexcept { engage(!engaged()); }

graphics::ScreenView update([[maybe_unused]] editor::SceneRef scene) {
#if defined(LEVK_EDITOR)
	if (!exists()) { return {}; }
	EXPECT(g_state.storage && Instance::Impl::s_impl);
	if (g_state.flags.test(Flag::eRenderReady)) { Instance::Impl::s_impl->imgui->endFrame(); }
	g_state.flags.assign(Flag::eRenderReady, Instance::Impl::s_impl->imgui->beginFrame());
	if (!g_state.flags.test(Flag::eRenderReady)) { return {}; }
	if (engaged()) {
		auto const& eng = Instance::Impl::s_impl->engine;
		if (!scene.valid() || g_state.cache.prev != editor::Sudo::registry(scene)) { g_state.cache = {}; }
		g_state.cache.prev = editor::Sudo::registry(scene);
		editor::Sudo::inspect(scene, g_state.cache.inspect);
		displayScale(eng.renderer().renderScale());
		if (!editor::Resizer::s_block) { g_state.storage->resizer(eng.window(), g_state.gameView, eng.inputFrame()); }
		editor::Resizer::s_block = false;
		g_state.storage->mainMenu(g_state.storage->menu);
		glm::vec2 const& size = eng.inputFrame().space.display.window;
		auto const rect = g_state.gameView.rect();
		f32 const offsetY = g_state.gameView.topLeft.offset.y;
		f32 const logHeight = size.y - rect.rb.y * size.y - offsetY;
		glm::vec2 const leftPanelSize = {rect.lt.x * size.x, size.y - logHeight - offsetY};
		glm::vec2 const rightPanelSize = {size.x - rect.rb.x * size.x, size.y - logHeight - offsetY};
		g_state.storage->logStats(size, logHeight);
		g_state.storage->left.tab->update(g_state.storage->left.id, leftPanelSize, {0.0f, offsetY}, scene);
		g_state.storage->right.tab->update(g_state.storage->right.id, rightPanelSize, {size.x - rightPanelSize.x, offsetY}, scene);
		return {g_state.gameView.rect(), g_state.gameView.topLeft.offset * eng.renderer().renderScale()};
	}
#endif
	return {};
}

void render(graphics::CommandBuffer const& cb) {
	if (exists() && g_state.flags.test(Flag::eRenderReady)) {
		EXPECT(Instance::Impl::s_impl);
		Instance::Impl::s_impl->imgui->endFrame();
		Instance::Impl::s_impl->imgui->renderDrawData(cb);
		g_state.flags.reset(Flag::eRenderReady);
	}
}
} // namespace le::editor
