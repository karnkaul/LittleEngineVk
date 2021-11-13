#include <core/maths.hpp>
#include <core/services.hpp>
#include <editor/sudo.hpp>
#include <engine/editor/editor.hpp>
#include <engine/editor/inspector.hpp>
#include <engine/editor/palette_tab.hpp>
#include <engine/editor/palettes/settings.hpp>
#include <engine/editor/scene_tree.hpp>
#include <engine/editor/types.hpp>
#include <engine/engine.hpp>
#include <graphics/context/bootstrap.hpp>
#include <graphics/geometry.hpp>
#include <graphics/render/renderer.hpp>

#define MU [[maybe_unused]]

namespace le {
namespace edi {
using sv = std::string_view;

void clicks(MU GUIState& out_state) {
#if defined(LEVK_USE_IMGUI)
	out_state.assign(GUI::eLeftClicked, ImGui::IsItemClicked(ImGuiMouseButton_Left));
	out_state.assign(GUI::eRightClicked, ImGui::IsItemClicked(ImGuiMouseButton_Right));
#endif
}

Styler::Styler(StyleFlags flags) : flags(flags) { (*this)(); }

Styler::Styler(MU glm::vec2 dummy) {
#if defined(LEVK_USE_IMGUI)
	ImGui::Dummy({dummy.x, dummy.y});
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

Button::Button(MU sv id) {
#if defined(LEVK_USE_IMGUI)
	refresh();
	guiState.assign(GUI::eLeftClicked, ImGui::Button(id.data()));
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
	if (guiState[GUI::eOpen] && blockResize) { s_blockResize = true; }
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

Popup::Popup(MU std::string_view id, MU int flags) {
#if defined(LEVK_USE_IMGUI)
	guiState.assign(GUI::eOpen, ImGui::BeginPopup(id.data(), flags));
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

TWidget<Colour>::TWidget(MU sv id, MU Colour& out_colour) {
#if defined(LEVK_USE_IMGUI)
	auto c = out_colour.toVec4();
	changed = ImGui::ColorEdit3(id.data(), &c.x);
	out_colour = Colour(c);
#endif
}

TWidget<char*>::TWidget(MU sv id, MU char* str, MU std::size_t size, MU f32 width, MU int flags) {
#if defined(LEVK_USE_IMGUI)
	if (width > 0.0f) { ImGui::SetNextItemWidth(width); }
	changed = ImGui::InputText(id.data(), str, size, flags);
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
} // namespace edi

Editor::Editor() {
	m_right.tab = std::make_unique<edi::EditorTab<edi::Inspector>>();
	auto left = std::make_unique<edi::EditorTab<edi::SceneTree>>();
	left->attach<edi::Settings>("Settings");
	m_left.tab = std::move(left);
}

Editor::~Editor() noexcept { edi::Sudo::clear<edi::Inspector, edi::SceneTree>(); }

bool Editor::active() const noexcept {
	if constexpr (levk_imgui) { return true; }
	return false;
}

Viewport const& Editor::view() const noexcept {
	static constexpr Viewport s_default;
	return active() && engaged() ? m_storage.gameView : s_default;
}

graphics::ScreenView Editor::update(MU edi::SceneRef scene, MU input::Frame const& frame) {
#if defined(LEVK_EDITOR)
	if (active() && engaged()) {
		if (!scene.valid() || m_cache.prev != edi::Sudo::registry(scene)) { m_cache = {}; }
		m_cache.prev = edi::Sudo::registry(scene);
		edi::Sudo::inspect(scene, m_cache.inspect);
		auto eng = Services::get<Engine>();
		edi::displayScale(eng->renderer().renderScale());
		if (!edi::Pane::s_blockResize) { m_storage.resizer(eng->window(), m_storage.gameView, frame); }
		edi::Pane::s_blockResize = false;
		m_storage.menu(m_menu);
		glm::vec2 const& size = frame.space.display.window;
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
} // namespace le
