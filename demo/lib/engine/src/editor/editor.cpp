#include <core/maths.hpp>
#include <engine/editor/controls/inspector.hpp>
#include <engine/editor/controls/scene_tree.hpp>
#include <engine/editor/editor.hpp>
#include <engine/editor/types.hpp>
#include <graphics/context/bootstrap.hpp>
#include <graphics/geometry.hpp>
#include <window/desktop_instance.hpp>

namespace le {
#if defined(LEVK_USE_IMGUI)
namespace edi {
using sv = std::string_view;

void clicks(GUIState& out_state) {
	out_state[GUI::eLeftClicked] = ImGui::IsItemClicked(ImGuiMouseButton_Left);
	out_state[GUI::eRightClicked] = ImGui::IsItemClicked(ImGuiMouseButton_Right);
}

Styler::Styler(StyleFlags flags) : flags(flags) {
	(*this)();
}

void Styler::operator()(std::optional<StyleFlags> f) {
	if (f) {
		flags = *f;
	}
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

Radio::Radio(View<sv> options, s32 preSelect, bool sameLine) : select(preSelect) {
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
}

Button::Button(sv id) {
	refresh();
	guiState[GUI::eLeftClicked] = ImGui::Button(id.empty() ? "[Unnamed]" : id.data());
}

Combo::Combo(sv id, View<sv> entries, sv preSelect) {
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
}

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

MenuBar::Menu::Menu(sv id) {
	guiState[GUI::eOpen] = ImGui::BeginMenu(id.data());
}

MenuBar::Menu::~Menu() {
	if (guiState[GUI::eOpen]) {
		ImGui::EndMenu();
	}
}

MenuBar::Item::Item(sv id, bool separator) {
	guiState[GUI::eLeftClicked] = ImGui::MenuItem(id.data());
	if (separator) {
		Styler s{Style::eSeparator};
	}
}

void MenuBar::walk(MenuList::Tree const& tree) {
	if (!tree.has_children()) {
		if (auto it = MenuBar::Item(tree.m_t.id, tree.m_t.separator); it && tree.m_t.callback) {
			tree.m_t.callback();
		}
	} else if (auto mb = Menu(tree.m_t.id)) {
		for (MenuList::Tree const& item : tree.children()) {
			walk(item);
		}
	}
}

MenuBar::MenuBar() {
	guiState[GUI::eOpen] = ImGui::BeginMenuBar();
}

MenuBar::MenuBar(MenuList const& list) {
	guiState.reset();
	if (ImGui::BeginMenuBar()) {
		for (MenuList::Tree const& tree : list.trees) {
			walk(tree);
		}
		ImGui::EndMenuBar();
	}
}

MenuBar::~MenuBar() {
	if (guiState[GUI::eOpen]) {
		ImGui::EndMenuBar();
	}
}

TabBar::TabBar(std::string_view id, s32 flags) {
	guiState[GUI::eOpen] = ImGui::BeginTabBar(id.data(), flags);
}

TabBar::~TabBar() {
	if (guiState[GUI::eOpen]) {
		ImGui::EndTabBar();
	}
}

TabBar::Item::Item(std::string_view id, s32 flags) {
	guiState[GUI::eOpen] = ImGui::BeginTabItem(id.data(), nullptr, flags);
}

TabBar::Item::~Item() {
	if (guiState[GUI::eOpen]) {
		ImGui::EndTabItem();
	}
}

Pane::Pane(std::string_view id, glm::vec2 size, glm::vec2 pos, bool child, s32 flags) : child(child) {
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
}

Pane::~Pane() {
	if (child) {
		ImGui::EndChild();
	} else {
		ImGui::End();
		if (guiState[GUI::eOpen]) {
			--s_open;
		}
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
			out_vec = graphics::front;
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
			out_vec = graphics::front;
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

TWidget<SceneNode>::TWidget(sv idPos, sv idOrn, sv idScl, SceneNode& out_t, glm::vec3 const& dPOS) {
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
}
} // namespace edi
#endif

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

void Editor::update(DesktopInstance& win, Input::State const& state) {
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
}
} // namespace le
