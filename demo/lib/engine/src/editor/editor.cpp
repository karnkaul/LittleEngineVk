#include <core/maths.hpp>
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

namespace {
#if defined(LEVK_USE_IMGUI)
void showDemo() {
	if (auto imgui = DearImGui::inst()) {
		imgui->m_showDemo = true;
	}
}

void walkMenu(edi::Menu const& menu) {
	if (ImGui::BeginMenu(menu.id.data())) {
		for (auto const& item : menu.items) {
			if (ImGui::MenuItem(item.id.data())) {
				if (item.callback) {
					item.callback();
				}
			}
			for (auto const& m : item.menus) {
				walkMenu(m);
			}
		}
		ImGui::EndMenu();
	}
}
#endif

void mainMenu([[maybe_unused]] View<edi::Menu> main) {
#if defined(LEVK_USE_IMGUI)
	if (ImGui::BeginMainMenuBar()) {
		for (auto const& menu : main) {
			walkMenu(menu);
		}
		ImGui::EndMainMenuBar();
	}
#endif
}
} // namespace

Editor::Editor() {
#if defined(LEVK_USE_IMGUI)
	edi::Menu::Item exit{"Close Editor", []() { Editor::s_engaged = false; }, {}};
	edi::Menu::Item demo{"Show ImGui Demo", []() { showDemo(); }, {}};
	edi::Menu file{"File", {demo, exit}};
	m_storage.main.push_back(std::move(file));
#endif
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
	if (active() && s_engaged) {
		m_storage.resizer(win, m_storage.gameView, state);
		mainMenu(m_storage.main);
		glm::vec2 const fbSize = {f32(win.framebufferSize().x), f32(win.framebufferSize().y)};
		auto const logHeight = fbSize.y - m_storage.gameView.rect().rb.y * fbSize.y - m_storage.gameView.topLeft.offset.y;
		m_storage.logStats(fbSize, logHeight);
	}
}
} // namespace le
