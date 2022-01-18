#include <editor/levk_imgui.hpp>
#include <editor/sudo.hpp>
#include <levk/core/maths.hpp>
#include <levk/core/services.hpp>
#include <levk/engine/editor/asset_index.hpp>
#include <levk/engine/editor/editor.hpp>
#include <levk/engine/editor/inspector.hpp>
#include <levk/engine/editor/palette_tab.hpp>
#include <levk/engine/editor/palettes/settings.hpp>
#include <levk/engine/editor/scene_tree.hpp>
#include <levk/engine/editor/types.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/render/model.hpp>
#include <levk/engine/render/pipeline.hpp>
#include <levk/engine/render/skybox.hpp>
#include <levk/engine/scene/asset_provider.hpp>
#include <levk/graphics/geometry.hpp>
#include <levk/graphics/render/renderer.hpp>

#define MU [[maybe_unused]]

namespace le {
namespace edi {
using sv = std::string_view;

f32 getWindowWidth() {
#if defined(LEVK_USE_IMGUI)
	return ImGui::GetWindowWidth();
#else
	return {};
#endif
}

void clicks(MU GUIState& out_state) {
#if defined(LEVK_USE_IMGUI)
	out_state.assign(GUI::eLeftClicked, ImGui::IsItemClicked(ImGuiMouseButton_Left));
	out_state.assign(GUI::eRightClicked, ImGui::IsItemClicked(ImGuiMouseButton_Right));
	if (out_state.any(GUIState(GUI::eLeftClicked, GUI::eRightClicked))) {
		out_state.assign(GUI::eDoubleClicked, ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) || ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right));
	} else if (ImGui::IsItemHovered()) {
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) { out_state.set(GUIState(GUI::eReleased, GUI::eLeftClicked)); }
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { out_state.set(GUIState(GUI::eReleased, GUI::eRightClicked)); }
	}
#endif
}

Styler::Styler(StyleFlags flags) : flags(flags) { (*this)(); }

Styler::Styler(MU glm::vec2 dummy) {
#if defined(LEVK_USE_IMGUI)
	ImGui::Dummy({dummy.x, dummy.y});
#endif
}

Styler::Styler(MU f32 sameLineX) {
#if defined(LEVK_USE_IMGUI)
	ImGui::SameLine(sameLineX);
#endif
}

void Styler::operator()(MU std::optional<StyleFlags> f) {
#if defined(LEVK_USE_IMGUI)
	if (f) { flags = *f; }
	if (flags.test(Style::eSameLine)) { ImGui::SameLine(); }
	if (flags.test(Style::eSeparator)) { ImGui::Separator(); }
#endif
}

GUIStateful::GUIStateful() { clicks(guiState); }

GUIStateful::GUIStateful(GUIStateful&& rhs) : guiState(std::exchange(rhs.guiState, GUIState{})) {}

GUIStateful& GUIStateful::operator=(GUIStateful&& rhs) {
	if (&rhs != this) { guiState = std::exchange(rhs.guiState, GUIState{}); }
	return *this;
}

void GUIStateful::refresh() { clicks(guiState); }

Text::Text(MU sv text) {
#if defined(LEVK_USE_IMGUI)
	ImGui::Text("%s", text.data());
#endif
}

Radio::Radio(MU Span<sv const> options, MU s32 preSelect, MU bool sameLine) : select(preSelect) {
#if defined(LEVK_USE_IMGUI)
	bool first = true;
	s32 idx = 0;
	for (auto text : options) {
		if (!first && sameLine) { Styler s(Style::eSameLine); }
		first = false;
		ImGui::RadioButton(text.data(), &select, idx++);
	}
	if (select >= 0) { selected = options[(std::size_t)select]; }
#endif
}

TreeSelect::Select TreeSelect::list(MU Span<Group const> groups, MU std::string_view selected) {
	Select ret;
#if defined(LEVK_USE_IMGUI)
	for (Group const& group : groups) {
		if (!group.id.empty()) {
			if (auto tn = TreeNode(group.id)) {
				for (std::string_view const item : group.items) {
					if (!item.empty()) {
						auto const asset = TreeNode(item, selected == item, true, true, false);
						if (asset.test(GUI::eLeftClicked)) { ret = {item, group.id}; }
					}
				}
			}
			Styler s(Style::eSeparator);
		}
	}
#endif
	return ret;
}

Button::Button(MU sv id, MU std::optional<f32> hue, MU bool small) {
#if defined(LEVK_USE_IMGUI)
	refresh();
	if (hue) {
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(*hue, 0.8f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(*hue, 0.8f, 0.8f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(*hue, 0.9f, 0.9f));
	}
	guiState.assign(GUI::eLeftClicked, small ? ImGui::SmallButton(id.data()) : ImGui::Button(id.data()));
	if (hue) { ImGui::PopStyleColor(3); }
#endif
}

Selectable::Selectable(MU sv id) {
#if defined(LEVK_USE_IMGUI)
	refresh();
	guiState.assign(GUI::eLeftClicked, ImGui::Selectable(id.data()));
#endif
}

Combo::Combo(MU sv id, MU Span<sv const> entries, MU sv preSelect) {
#if defined(LEVK_USE_IMGUI)
	if (!entries.empty()) {
		guiState.assign(GUI::eOpen, ImGui::BeginCombo(id.data(), preSelect.data()));
		refresh();
		if (test(GUI::eOpen)) {
			std::size_t i = 0;
			for (auto entry : entries) {
				bool const bSelected = preSelect == entry;
				if (ImGui::Selectable(entry.data(), bSelected)) {
					select = (s32)i;
					selected = entry;
				}
				if (bSelected) { ImGui::SetItemDefaultFocus(); }
				++i;
			}
			ImGui::EndCombo();
		}
	}
#endif
}

TreeNode::TreeNode(MU sv id) {
#if defined(LEVK_USE_IMGUI)
	guiState.assign(GUI::eOpen, ImGui::TreeNode(id.data()));
	refresh();
#endif
}

TreeNode::TreeNode(MU sv id, MU bool bSelected, MU bool bLeaf, MU bool bFullWidth, MU bool bLeftClickOpen) {
#if defined(LEVK_USE_IMGUI)
	static constexpr ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	ImGuiTreeNodeFlags const branchFlags = (bLeftClickOpen ? 0 : ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick);
	ImGuiTreeNodeFlags const metaFlags = (bSelected ? ImGuiTreeNodeFlags_Selected : 0) | (bFullWidth ? ImGuiTreeNodeFlags_SpanAvailWidth : 0);
	ImGuiTreeNodeFlags const nodeFlags = (bLeaf ? leafFlags : branchFlags) | metaFlags;
	guiState.assign(GUI::eOpen, ImGui::TreeNodeEx(id.data(), nodeFlags) && !bLeaf);
	refresh();
#endif
}

TreeNode::~TreeNode() {
#if defined(LEVK_USE_IMGUI)
	if (test(GUI::eOpen)) { ImGui::TreePop(); }
#endif
}

MenuBar::Menu::Menu(MU sv id) {
#if defined(LEVK_USE_IMGUI)
	guiState.assign(GUI::eOpen, ImGui::BeginMenu(id.data()));
#endif
}

MenuBar::Menu::~Menu() {
#if defined(LEVK_USE_IMGUI)
	if (guiState[GUI::eOpen]) { ImGui::EndMenu(); }
#endif
}

MenuBar::Item::Item(MU sv id, MU bool separator) {
#if defined(LEVK_USE_IMGUI)
	guiState.assign(GUI::eLeftClicked, ImGui::MenuItem(id.data()));
	if (separator) { Styler s{Style::eSeparator}; }
#endif
}

void MenuBar::walk(MU MenuList::Tree const& tree) {
#if defined(LEVK_USE_IMGUI)
	if (!tree.has_children()) {
		if (auto it = MenuBar::Item(tree.m_t.id, tree.m_t.separator); it && tree.m_t.callback) { tree.m_t.callback(); }
	} else if (auto mb = Menu(tree.m_t.id)) {
		for (MenuList::Tree const& item : tree.children()) { walk(item); }
	}
#endif
}

MenuBar::MenuBar() {
#if defined(LEVK_USE_IMGUI)
	guiState.assign(GUI::eOpen, ImGui::BeginMenuBar());
#endif
}

MenuBar::MenuBar(MU MenuList const& list) {
#if defined(LEVK_USE_IMGUI)
	guiState.reset();
	if (ImGui::BeginMenuBar()) {
		for (MenuList::Tree const& tree : list.trees) { walk(tree); }
		ImGui::EndMenuBar();
	}
#endif
}

MenuBar::~MenuBar() {
#if defined(LEVK_USE_IMGUI)
	if (guiState[GUI::eOpen]) { ImGui::EndMenuBar(); }
#endif
}

TabBar::TabBar(MU std::string_view id, MU s32 flags) {
#if defined(LEVK_USE_IMGUI)
	guiState.assign(GUI::eOpen, ImGui::BeginTabBar(id.data(), flags));
#endif
}

TabBar::~TabBar() {
#if defined(LEVK_USE_IMGUI)
	if (guiState[GUI::eOpen]) { ImGui::EndTabBar(); }
#endif
}

TabBar::Item::Item(MU std::string_view id, MU s32 flags) {
#if defined(LEVK_USE_IMGUI)
	guiState.assign(GUI::eOpen, ImGui::BeginTabItem(id.data(), nullptr, flags));
#endif
}

TabBar::Item::~Item() {
#if defined(LEVK_USE_IMGUI)
	if (guiState[GUI::eOpen]) { ImGui::EndTabItem(); }
#endif
}

Pane::Pane(MU std::string_view id, MU glm::vec2 size, MU glm::vec2 pos, MU bool* open, MU bool blockResize, MU s32 flags) : child(false) {
#if defined(LEVK_USE_IMGUI)
	ImGui::SetNextWindowSize({size.x, size.y}, ImGuiCond_Once);
	ImGui::SetNextWindowPos({pos.x, pos.y}, ImGuiCond_Once);
	guiState.assign(GUI::eOpen, ImGui::Begin(id.data(), open, flags));
	if (guiState[GUI::eOpen] && blockResize) { Resizer::s_block = true; }
#endif
}

Pane::Pane(MU std::string_view id, MU glm::vec2 size, MU bool border, MU s32 flags) : child(true) {
#if defined(LEVK_USE_IMGUI)
	guiState.assign(GUI::eOpen, ImGui::BeginChild(id.data(), {size.x, size.y}, border, flags));
#endif
}

Pane::~Pane() {
#if defined(LEVK_USE_IMGUI)
	if (child) {
		ImGui::EndChild();
	} else {
		ImGui::End();
	}
#endif
}

Popup::Popup(MU std::string_view id, MU bool blockResize, MU int flags) {
#if defined(LEVK_USE_IMGUI)
	guiState.assign(GUI::eOpen, ImGui::BeginPopup(id.data(), flags));
	if (guiState[GUI::eOpen] && blockResize) { Resizer::s_block = true; }
#endif
}

Popup::~Popup() {
#if defined(LEVK_USE_IMGUI)
	if (*this) { ImGui::EndPopup(); }
#endif
}

void Popup::open(MU std::string_view id) {
#if defined(LEVK_USE_IMGUI)
	ImGui::OpenPopup(id.data());
#endif
}

void Popup::close() {
#if defined(LEVK_USE_IMGUI)
	if (*this) { ImGui::CloseCurrentPopup(); }
#endif
}

DragDrop::Source::Source(MU int flags) {
#if defined(LEVK_USE_IMGUI)
	begun = ImGui::BeginDragDropSource(flags);
#endif
}

DragDrop::Source::~Source() {
#if defined(LEVK_USE_IMGUI)
	if (begun) { ImGui::EndDragDropSource(); }
#endif
}

void DragDrop::Source::payload(MU std::string_view type, MU Payload payload) const {
#if defined(LEVK_USE_IMGUI)
	ImGui::SetDragDropPayload(type.data(), payload.data, payload.size);
#endif
}

DragDrop::Target::Target() {
#if defined(LEVK_USE_IMGUI)
	begun = ImGui::BeginDragDropTarget();
#endif
}

DragDrop::Target::~Target() {
#if defined(LEVK_USE_IMGUI)
	if (begun) { ImGui::EndDragDropTarget(); }
#endif
}

Payload DragDrop::Target::rawPayload(MU std::string_view type) const {
#if defined(LEVK_USE_IMGUI)
	if (ImGuiPayload const* p = ImGui::AcceptDragDropPayload(type.data())) { return {p->Data, std::size_t(p->DataSize)}; }
#endif
	return {};
}

TWidget<bool>::TWidget(MU sv id, MU bool& out_b) {
#if defined(LEVK_USE_IMGUI)
	changed = ImGui::Checkbox(id.data(), &out_b);
#endif
}

TWidget<int>::TWidget(MU sv id, MU int& out_s, MU f32 w, MU glm::ivec2 rng, MU WType wt) {
#if defined(LEVK_USE_IMGUI)
	if (w > 0.0f) { ImGui::SetNextItemWidth(w); }
	switch (wt) {
	case WType::eDrag: changed = ImGui::DragInt(id.data(), &out_s, 0.1f, rng.x, rng.y); break;
	default: changed = ImGui::InputInt(id.data(), &out_s); break;
	}
#endif
}

TWidget<f32>::TWidget(MU sv id, MU f32& out_f, MU f32 df, MU f32 w, MU glm::vec2 rng, MU WType wt) {
#if defined(LEVK_USE_IMGUI)
	if (w > 0.0f) { ImGui::SetNextItemWidth(w); }
	switch (wt) {
	case WType::eDrag: changed = ImGui::DragFloat(id.data(), &out_f, df, rng.x, rng.y); break;
	default: changed = ImGui::InputFloat(id.data(), &out_f, df); break;
	}
#endif
}

TWidget<char*>::TWidget(MU sv id, MU char* str, MU std::size_t size, MU f32 width, MU int flags) {
#if defined(LEVK_USE_IMGUI)
	if (width > 0.0f) { ImGui::SetNextItemWidth(width); }
	changed = ImGui::InputText(id.data(), str, size, flags);
#endif
}

TWidget<std::string_view>::TWidget(MU sv id, MU sv readonly, MU f32 width, MU int flags) {
#if defined(LEVK_USE_IMGUI)
	if (width > 0.0f) { ImGui::SetNextItemWidth(width); }
	changed = ImGui::InputText(id.data(), (char*)readonly.data(), readonly.size(), flags | ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
#endif
}

TWidget<Colour>::TWidget(MU sv id, MU Colour& out_colour, MU bool alpha, MU bool input) {
#if defined(LEVK_USE_IMGUI)
	ImGuiColorEditFlags const flags = input ? 0 : ImGuiColorEditFlags_NoInputs;
	auto c = out_colour.toVec4();
	changed = alpha ? ImGui::ColorEdit4(id.data(), &c.x, flags) : ImGui::ColorEdit3(id.data(), &c.x, flags);
	out_colour = Colour(c);
#endif
}

TWidget<graphics::RGBA>::TWidget(MU sv id, MU graphics::RGBA& out_colour, MU bool alpha) {
#if defined(LEVK_USE_IMGUI)
	ImGuiColorEditFlags const flags = ImGuiColorEditFlags_NoInputs;
	auto c = out_colour.colour.toVec4();
	bool abs = out_colour.type == graphics::RGBA::Type::eAbsolute;
	changed |= alpha ? ImGui::ColorEdit4(id.data(), &c.x, flags) : ImGui::ColorEdit3(id.data(), &c.x, flags);
	Styler s(Style::eSameLine);
	changed |= ImGui::Checkbox("Absolute", &abs);
	if (changed) { out_colour = graphics::RGBA(c, abs ? graphics::RGBA::Type::eAbsolute : graphics::RGBA::Type::eIntensity); }
#endif
}

TWidget<glm::vec2>::TWidget(MU sv id, MU glm::vec2& out_vec, MU bool bNormalised, MU f32 dv) {
#if defined(LEVK_USE_IMGUI)
	if (bNormalised) {
		if (glm::length2(out_vec) <= 0.0f) {
			out_vec = graphics::front;
		} else {
			out_vec = glm::normalize(out_vec);
		}
	}
	changed = ImGui::DragFloat2(id.data(), &out_vec.x, dv);
	if (bNormalised) { out_vec = glm::normalize(out_vec); }
#endif
}

TWidget<glm::vec3>::TWidget(MU sv id, MU glm::vec3& out_vec, MU bool bNormalised, MU f32 dv) {
#if defined(LEVK_USE_IMGUI)
	if (bNormalised) {
		if (glm::length2(out_vec) <= 0.0f) {
			out_vec = graphics::front;
		} else {
			out_vec = glm::normalize(out_vec);
		}
	}
	changed = ImGui::DragFloat3(id.data(), &out_vec.x, dv);
	if (bNormalised) { out_vec = glm::normalize(out_vec); }
#endif
}

TWidget<glm::quat>::TWidget(MU sv id, MU glm::quat& out_quat, MU f32 dq) {
#if defined(LEVK_USE_IMGUI)
	auto rot = glm::eulerAngles(out_quat);
	changed = ImGui::DragFloat3(id.data(), &rot.x, dq);
	out_quat = glm::quat(rot);
#endif
}

TWidget<Transform>::TWidget(MU sv idPos, MU sv idOrn, MU sv idScl, MU Transform& out_t, MU glm::vec3 const& dPOS) {
#if defined(LEVK_USE_IMGUI)
	auto posn = out_t.position();
	auto scl = out_t.scale();
	auto const& orn = out_t.orientation();
	auto rot = glm::eulerAngles(orn);
	changed = ImGui::DragFloat3(idPos.data(), &posn.x, dPOS.x);
	out_t.position(posn);
	changed |= ImGui::DragFloat3(idOrn.data(), &rot.x, dPOS.y);
	out_t.orient(glm::quat(rot));
	changed |= ImGui::DragFloat3(idScl.data(), &scl.x, dPOS.z);
	out_t.scale(scl);
#endif
}

TWidget<std::pair<s64, s64>>::TWidget(MU sv id, MU s64& out_t, MU s64 min, MU s64 max, MU s64 dt) {
#if defined(LEVK_USE_IMGUI)
	ImGui::PushButtonRepeat(true);
	if (ImGui::ArrowButton(CStr<64>("##%s_left", id.data()).data(), ImGuiDir_Left) && out_t > min) {
		out_t -= dt;
		changed = true;
	}
	ImGui::SameLine(0.0f, 3.0f);
	if (ImGui::ArrowButton(CStr<64>("##%s_right", id.data()).data(), ImGuiDir_Right) && out_t < max) {
		out_t += dt;
		changed = true;
	}
	ImGui::PopButtonRepeat();
	ImGui::SameLine(0.0f, 5.0f);
	ImGui::Text("%s", id.data());
#endif
}

void displayScale(MU f32 renderScale) {
#if defined(LEVK_USE_IMGUI)
	auto& ds = ImGui::GetIO().DisplayFramebufferScale;
	ds = {ds.x * renderScale, ds.y * renderScale};
#endif
}

template <typename T>
class EditorTab : public PaletteTab {
	void loopItems(SceneRef scene) override {
		if (auto it = TabBar::Item(T::title_v)) { Sudo::update<T>(scene); }
		PaletteTab::loopItems(scene);
	}
};

void inspectMat(Material* out_mat, std::string_view name, int idx) {
	auto const id = idx >= 0 ? ktl::stack_string<64>("Material_%d", idx) : ktl::stack_string<64>("Material");
	if (auto tn = TreeNode(id, false, false, true, true)) {
		if (!name.empty()) { Selectable name_(name); }
		if (out_mat) {
			TWidget<graphics::RGBA> Tf("Tf", out_mat->Tf, true);
			TWidget<f32> d("d", out_mat->d);
		}
	}
}

void inspectMP(Inspect<MeshProvider> provider) {
	std::string_view type = "Other";
	auto const th = provider.get().sign();
	if (th == AssetStore::sign<MeshPrimitive>()) {
		type = "Mesh Primitive";
	} else if (th == AssetStore::sign<Model>()) {
		type = "Model";
	} else if (th == AssetStore::sign<Skybox>()) {
		type = "Skybox";
	}
	auto store = Services::find<AssetStore>();
	Text typeStr(ktl::stack_string<64>("Type: %s", type.data()));
	Text uri(provider.get().assetURI());
	if (store) {
		if (type == "Mesh Primitive") {
			std::string_view const matURI = provider.get().materialURI();
			inspectMat(store->find<Material>(matURI).peek(), matURI, -1);
		} else if (type == "Model") {
			if (auto model = store->find<Model>(provider.get().assetURI())) {
				for (std::size_t i = 0; i < model->materialCount(); ++i) { inspectMat(model->material(i), {}, int(i)); }
			}
		}
	}
	static ktl::stack_string<128> s_search;
	if (auto popup = Popup("Model##inspect_asset_provider")) {
		TWidget<char*> search("Search##inspect_asset_provider", s_search.c_str(), s_search.capacity());
		if (auto select = AssetIndex::list<Model>(s_search, s_search)) {
			provider.get() = MeshProvider::make<Model>(std::string(select.item));
			popup.close();
		}
	}
	if (auto popup = Popup("Skybox##inspect_asset_provider")) {
		TWidget<char*> search("Search##inspect_asset_provider", s_search.c_str(), s_search.capacity());
		if (auto select = AssetIndex::list<Skybox>(s_search, s_search)) {
			provider.get() = MeshProvider::make<Skybox>(std::string(select.item));
			popup.close();
		}
	}
	{
		static std::string_view s_mesh, s_mat = "materials/default";
		if (auto popup = Popup("Mesh Primitive##inspect_asset_provider")) {
			TWidget<char*> search("Search##inspect_asset_provider", s_search.c_str(), s_search.capacity());
			if (auto select = AssetIndex::list<MeshPrimitive>(s_search, s_mesh)) { s_mesh = select.item; }
			if (auto select = AssetIndex::list<Material>(s_search, s_mat)) { s_mat = select.item; }
			if (!s_mesh.empty() && !s_mat.empty() && Button("OK")) {
				provider.get() = provider.get().make(std::string(s_mesh), std::string(s_mat));
				popup.close();
			}
		}
		std::string_view toPopup;
		if (auto popup = Popup("Type##inspect_asset_provider")) {
			if (Selectable("Mesh Primitive")) {
				toPopup = "Mesh Primitive##inspect_asset_provider";
				popup.close();
			}
			if (Selectable("Model")) {
				toPopup = "Model##inspect_asset_provider";
				popup.close();
			}
		}
		if (!toPopup.empty()) {
			s_mat = {};
			s_mesh = {};
			Popup::open(toPopup);
		}
	}
	if (Button("Edit...")) {
		if (type == "Skybox") {
			Popup::open("Skybox##inspect_asset_provider");
		} else {
			Popup::open("Type##inspect_asset_provider");
		}
	}
}

void inspectRLP(Inspect<RenderPipeProvider> provider) {
	Text uri(provider.get().uri());
	if (auto popup = Popup("RenderPipeline##inspect_pipe_provider")) {
		static ktl::stack_string<128> s_search;
		TWidget<char*> search("Search##inspect_pipe_provider", s_search.c_str(), s_search.capacity());
		if (auto select = AssetIndex::list<RenderPipeline>(s_search)) {
			provider.get() = RenderPipeProvider::make(std::string(select.item));
			popup.close();
		}
	}
	if (Button("Edit...")) { Popup::open("RenderPipeline##inspect_pipe_provider"); }
}
} // namespace edi

Editor::Editor() {
	m_left.tab = std::make_unique<edi::EditorTab<edi::SceneTree>>();
	m_left.tab->attach<edi::Settings>("Settings");
	m_right.tab = std::make_unique<edi::EditorTab<edi::Inspector>>();
	edi::Inspector::attach<MeshProvider>(&edi::inspectMP, {}, "Mesh");
	edi::Inspector::attach<RenderPipeProvider>(&edi::inspectRLP, {}, "RenderPipeline");
}

Editor::~Editor() noexcept { edi::Sudo::clear<edi::Inspector, edi::SceneTree>(); }

bool Editor::active() const noexcept {
	if constexpr (levk_imgui) { return true; }
	return false;
}

bool Editor::showImGuiDemo() const noexcept {
	if (active() && m_imgui) {
		m_imgui->m_showDemo = true;
		return true;
	}
	return false;
}

Viewport const& Editor::view() const noexcept {
	static constexpr Viewport s_default;
	return active() && engaged() ? m_storage.gameView : s_default;
}

graphics::ScreenView Editor::update(MU edi::SceneRef scene) {
#if defined(LEVK_EDITOR)
	auto eng = Services::find<Engine::Service>();
	if (active() && engaged() && eng) {
		if (!scene.valid() || m_cache.prev != edi::Sudo::registry(scene)) { m_cache = {}; }
		m_cache.prev = edi::Sudo::registry(scene);
		edi::Sudo::inspect(scene, m_cache.inspect);
		edi::displayScale(eng->renderer().renderScale());
		if (!edi::Resizer::s_block) { m_storage.resizer(eng->window(), m_storage.gameView, eng->inputFrame()); }
		edi::Resizer::s_block = false;
		m_storage.menu(m_menu);
		glm::vec2 const& size = eng->inputFrame().space.display.window;
		auto const rect = m_storage.gameView.rect();
		f32 const offsetY = m_storage.gameView.topLeft.offset.y;
		f32 const logHeight = size.y - rect.rb.y * size.y - offsetY;
		glm::vec2 const leftPanelSize = {rect.lt.x * size.x, size.y - logHeight - offsetY};
		glm::vec2 const rightPanelSize = {size.x - rect.rb.x * size.x, size.y - logHeight - offsetY};
		m_storage.logStats(size, logHeight);
		m_left.tab->update(m_left.id, leftPanelSize, {0.0f, offsetY}, scene);
		m_right.tab->update(m_right.id, rightPanelSize, {size.x - rightPanelSize.x, offsetY}, scene);
		return {m_storage.gameView.rect(), m_storage.gameView.topLeft.offset * eng->renderer().renderScale()};
	}
#endif
	return {};
}

void Editor::init(graphics::RenderContext& context, window::Window const& window) {
	if constexpr (levk_imgui) {
		if (!m_imgui) { m_imgui = std::make_unique<DearImGui>(); }
		if (!m_imgui->init(context, window)) { logW(LC_LibUser, "[Editor] Failed to initialize Dear ImGui"); }
	}
}

void Editor::deinit() noexcept {
	if constexpr (levk_imgui) { m_imgui.reset(); }
}

bool Editor::beginFrame() { return m_imgui ? m_imgui->beginFrame() : false; }

void Editor::render(graphics::CommandBuffer const& cb) {
	if (m_imgui) {
		m_imgui->endFrame();
		m_imgui->renderDrawData(cb);
	}
}
} // namespace le