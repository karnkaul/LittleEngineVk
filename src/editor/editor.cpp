#include <core/maths.hpp>
#include <engine/editor/controls/inspector.hpp>
#include <engine/editor/controls/scene_tree.hpp>
#include <engine/editor/editor.hpp>
#include <engine/editor/types.hpp>
#include <graphics/context/bootstrap.hpp>
#include <graphics/geometry.hpp>
#include <window/desktop_instance.hpp>

#define MU [[maybe_unused]]

namespace le {
namespace edi {
using sv = std::string_view;

void clicks(MU GUIState& out_state) {
#if defined(LEVK_USE_IMGUI)
	out_state[GUI::eLeftClicked] = ImGui::IsItemClicked(ImGuiMouseButton_Left);
	out_state[GUI::eRightClicked] = ImGui::IsItemClicked(ImGuiMouseButton_Right);
#endif
}

Styler::Styler(StyleFlags flags) : flags(flags) {
	(*this)();
}

void Styler::operator()(MU std::optional<StyleFlags> f) {
#if defined(LEVK_USE_IMGUI)
	if (f) {
		flags = *f;
	}
	if (flags.test(Style::eSameLine)) {
		ImGui::SameLine();
	}
	if (flags.test(Style::eSeparator)) {
		ImGui::Separator();
	}
#endif
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

Text::Text(MU sv text) {
#if defined(LEVK_USE_IMGUI)
	ImGui::Text("%s", text.data());
#endif
}

Radio::Radio(MU View<sv> options, MU s32 preSelect, MU bool sameLine) : select(preSelect) {
#if defined(LEVK_USE_IMGUI)
	bool first = true;
	s32 idx = 0;
	for (auto text : options) {
		if (!first && sameLine) {
			Styler s(Style::eSameLine);
		}
		first = false;
		ImGui::RadioButton(text.data(), &select, idx++);
	}
	if (select >= 0) {
		selected = options[(std::size_t)select];
	}
#endif
}

Button::Button(MU sv id) {
#if defined(LEVK_USE_IMGUI)
	refresh();
	guiState[GUI::eLeftClicked] = ImGui::Button(id.empty() ? "[Unnamed]" : id.data());
#endif
}

Combo::Combo(MU sv id, MU View<sv> entries, MU sv preSelect) {
#if defined(LEVK_USE_IMGUI)
	if (!entries.empty()) {
		guiState[GUI::eOpen] = ImGui::BeginCombo(id.empty() ? "[Unnamed]" : id.data(), preSelect.data());
		refresh();
		if (test(GUI::eOpen)) {
			std::size_t i = 0;
			for (auto entry : entries) {
				bool const bSelected = preSelect == entry;
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
#endif
}

TreeNode::TreeNode(MU sv id) {
#if defined(LEVK_USE_IMGUI)
	guiState[GUI::eOpen] = ImGui::TreeNode(id.empty() ? "[Unnamed]" : id.data());
	refresh();
#endif
}

TreeNode::TreeNode(MU sv id, MU bool bSelected, MU bool bLeaf, MU bool bFullWidth, MU bool bLeftClickOpen) {
#if defined(LEVK_USE_IMGUI)
	static constexpr ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	ImGuiTreeNodeFlags const branchFlags = (bLeftClickOpen ? 0 : ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick);
	ImGuiTreeNodeFlags const metaFlags = (bSelected ? ImGuiTreeNodeFlags_Selected : 0) | (bFullWidth ? ImGuiTreeNodeFlags_SpanAvailWidth : 0);
	ImGuiTreeNodeFlags const nodeFlags = (bLeaf ? leafFlags : branchFlags) | metaFlags;
	guiState[GUI::eOpen] = ImGui::TreeNodeEx(id.empty() ? "[Unnamed]" : id.data(), nodeFlags) && !bLeaf;
	refresh();
#endif
}

TreeNode::~TreeNode() {
#if defined(LEVK_USE_IMGUI)
	if (test(GUI::eOpen)) {
		ImGui::TreePop();
	}
#endif
}

MenuBar::Menu::Menu(MU sv id) {
#if defined(LEVK_USE_IMGUI)
	guiState[GUI::eOpen] = ImGui::BeginMenu(id.data());
#endif
}

MenuBar::Menu::~Menu() {
#if defined(LEVK_USE_IMGUI)
	if (guiState[GUI::eOpen]) {
		ImGui::EndMenu();
	}
#endif
}

MenuBar::Item::Item(MU sv id, MU bool separator) {
#if defined(LEVK_USE_IMGUI)
	guiState[GUI::eLeftClicked] = ImGui::MenuItem(id.data());
	if (separator) {
		Styler s{Style::eSeparator};
	}
#endif
}

void MenuBar::walk(MU MenuList::Tree const& tree) {
#if defined(LEVK_USE_IMGUI)
	if (!tree.has_children()) {
		if (auto it = MenuBar::Item(tree.m_t.id, tree.m_t.separator); it && tree.m_t.callback) {
			tree.m_t.callback();
		}
	} else if (auto mb = Menu(tree.m_t.id)) {
		for (MenuList::Tree const& item : tree.children()) {
			walk(item);
		}
	}
#endif
}

MenuBar::MenuBar() {
#if defined(LEVK_USE_IMGUI)
	guiState[GUI::eOpen] = ImGui::BeginMenuBar();
#endif
}

MenuBar::MenuBar(MU MenuList const& list) {
#if defined(LEVK_USE_IMGUI)
	guiState.reset();
	if (ImGui::BeginMenuBar()) {
		for (MenuList::Tree const& tree : list.trees) {
			walk(tree);
		}
		ImGui::EndMenuBar();
	}
#endif
}

MenuBar::~MenuBar() {
#if defined(LEVK_USE_IMGUI)
	if (guiState[GUI::eOpen]) {
		ImGui::EndMenuBar();
	}
#endif
}

TabBar::TabBar(MU std::string_view id, MU s32 flags) {
#if defined(LEVK_USE_IMGUI)
	guiState[GUI::eOpen] = ImGui::BeginTabBar(id.data(), flags);
#endif
}

TabBar::~TabBar() {
#if defined(LEVK_USE_IMGUI)
	if (guiState[GUI::eOpen]) {
		ImGui::EndTabBar();
	}
#endif
}

TabBar::Item::Item(MU std::string_view id, MU s32 flags) {
#if defined(LEVK_USE_IMGUI)
	guiState[GUI::eOpen] = ImGui::BeginTabItem(id.data(), nullptr, flags);
#endif
}

TabBar::Item::~Item() {
#if defined(LEVK_USE_IMGUI)
	if (guiState[GUI::eOpen]) {
		ImGui::EndTabItem();
	}
#endif
}

Pane::Pane(MU std::string_view id, MU glm::vec2 size, MU glm::vec2 pos, MU bool child, MU s32 flags) : child(child) {
#if defined(LEVK_USE_IMGUI)
	if (child) {
		guiState[GUI::eOpen] = ImGui::BeginChild(id.data(), {size.x, size.y}, false, flags);
	} else {
		ImGui::SetNextWindowSize({size.x, size.y}, ImGuiCond_Always);
		ImGui::SetNextWindowPos({pos.x, pos.y}, ImGuiCond_Always);
		guiState[GUI::eOpen] = ImGui::Begin(id.data(), &open, flags);
		if (guiState[GUI::eOpen]) {
			++s_open;
		}
	}
#endif
}

Pane::~Pane() {
#if defined(LEVK_USE_IMGUI)
	if (child) {
		ImGui::EndChild();
	} else {
		ImGui::End();
		if (guiState[GUI::eOpen]) {
			--s_open;
		}
	}
#endif
}

TWidget<bool>::TWidget(MU sv id, MU bool& out_b) {
#if defined(LEVK_USE_IMGUI)
	ImGui::Checkbox(id.empty() ? "[Unnamed]" : id.data(), &out_b);
#endif
}

TWidget<s32>::TWidget(MU sv id, MU s32& out_s, MU f32 w) {
#if defined(LEVK_USE_IMGUI)
	if (w > 0.0f) {
		ImGui::SetNextItemWidth(w);
	}
	ImGui::DragInt(id.empty() ? "[Unnamed]" : id.data(), &out_s);
#endif
}

TWidget<f32>::TWidget(MU sv id, MU f32& out_f, MU f32 df, MU f32 w) {
#if defined(LEVK_USE_IMGUI)
	if (w > 0.0f) {
		ImGui::SetNextItemWidth(w);
	}
	ImGui::DragFloat(id.empty() ? "[Unnamed]" : id.data(), &out_f, df);
#endif
}

TWidget<Colour>::TWidget(MU sv id, MU Colour& out_colour) {
#if defined(LEVK_USE_IMGUI)
	auto c = out_colour.toVec4();
	ImGui::ColorEdit3(id.empty() ? "[Unnamed]" : id.data(), &c.x);
	out_colour = Colour(c);
#endif
}

TWidget<std::string>::TWidget(MU sv id, MU ZeroedBuf& out_buf, MU f32 width, MU std::size_t max) {
#if defined(LEVK_USE_IMGUI)
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
	ImGui::DragFloat2(id.empty() ? "[Unnamed]" : id.data(), &out_vec.x, dv);
	if (bNormalised) {
		out_vec = glm::normalize(out_vec);
	}
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
	ImGui::DragFloat3(id.empty() ? "[Unnamed]" : id.data(), &out_vec.x, dv);
	if (bNormalised) {
		out_vec = glm::normalize(out_vec);
	}
#endif
}

TWidget<glm::quat>::TWidget(MU sv id, MU glm::quat& out_quat, MU f32 dq) {
#if defined(LEVK_USE_IMGUI)
	auto rot = glm::eulerAngles(out_quat);
	ImGui::DragFloat3(id.empty() ? "[Unnamed]" : id.data(), &rot.x, dq);
	out_quat = glm::quat(rot);
#endif
}

TWidget<SceneNode>::TWidget(MU sv idPos, MU sv idOrn, MU sv idScl, MU SceneNode& out_t, MU glm::vec3 const& dPOS) {
#if defined(LEVK_USE_IMGUI)
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
#endif
}

TWidget<std::pair<s64, s64>>::TWidget(MU sv id, MU s64& out_t, MU s64 min, MU s64 max, MU s64 dt) {
#if defined(LEVK_USE_IMGUI)
	ImGui::PushButtonRepeat(true);
	if (ImGui::ArrowButton(fmt::format("##{}_left", id).data(), ImGuiDir_Left) && out_t > min) {
		out_t -= dt;
	}
	ImGui::SameLine(0.0f, 3.0f);
	if (ImGui::ArrowButton(fmt::format("##{}_right", id).data(), ImGuiDir_Right) && out_t < max) {
		out_t += dt;
	}
	ImGui::PopButtonRepeat();
	ImGui::SameLine(0.0f, 5.0f);
	ImGui::Text("%s", id.data());
#endif
}
} // namespace edi

Editor::Editor() {
	s_left.panel.attach<edi::SceneTree>("Scene");
	s_right.panel.attach<edi::Inspector>("Inspector");
}

bool Editor::active() const noexcept {
	if constexpr (levk_imgui) {
		return DearImGui::inst() != nullptr;
	}
	return false;
}

Viewport const& Editor::view() const noexcept {
	static constexpr Viewport s_default;
	return active() && s_engaged ? m_storage.gameView : s_default;
}

void Editor::update([[maybe_unused]] DesktopInstance& win, [[maybe_unused]] Input::State const& state) {
#if defined(LEVK_DESKTOP)
	if (m_storage.cached.root != s_in.root || m_storage.cached.registry != s_in.registry) {
		s_out = {};
	}
	if (!s_in.registry || !s_in.registry->contains(s_out.inspecting.entity)) {
		s_out.inspecting = {};
	}
	if (active() && s_engaged) {
		if (edi::Pane::s_open == 0) {
			m_storage.resizer(win, m_storage.gameView, state);
		}
		m_storage.menu(s_in.menu);
		glm::vec2 const fbSize = {f32(win.framebufferSize().x), f32(win.framebufferSize().y)};
		auto const rect = m_storage.gameView.rect();
		f32 const offsetY = m_storage.gameView.topLeft.offset.y;
		f32 const logHeight = fbSize.y - rect.rb.y * fbSize.y - offsetY;
		glm::vec2 const leftPanelSize = {rect.lt.x * fbSize.x, fbSize.y - logHeight - offsetY};
		glm::vec2 const rightPanelSize = {fbSize.x - rect.rb.x * fbSize.x, fbSize.y - logHeight - offsetY};
		m_storage.logStats(fbSize, logHeight);
		s_left.panel.update(s_left.id, leftPanelSize, {0.0f, offsetY});
		s_right.panel.update(s_right.id, rightPanelSize, {fbSize.x - rightPanelSize.x, offsetY});
		m_storage.cached = std::move(s_in);
		s_in = {};
	}
#endif
}
} // namespace le
