#include <imgui.h>
#include <le/core/fixed_string.hpp>
#include <le/engine.hpp>
#include <le/imcpp/common.hpp>
#include <cassert>

namespace le::imcpp {
auto max_size(std::span<char const* const> strings) -> glm::vec2 {
	auto ret = glm::vec2{};
	for (auto const* str : strings) {
		auto const size = ImGui::CalcTextSize(str);
		ret.x = std::max(ret.x, size.x);
		ret.y = std::max(ret.y, size.y);
	}
	return ret;
}

auto small_button_red(char const* label) -> bool {
	bool ret = false;
	ImGui::PushStyleColor(ImGuiCol_Button, {0.8f, 0.0f, 0.1f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1.0f, 0.0f, 0.1f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.6f, 0.0f, 0.1f, 1.0f});
	if (ImGui::SmallButton(label)) { ret = true; }
	ImGui::PopStyleColor(3);
	return ret;
}

auto selectable(char const* label, bool selected, int flags, glm::vec2 size) -> bool { return ImGui::Selectable(label, selected, flags, {size.x, size.y}); }

auto input_text(char const* label, char* buffer, std::size_t size, int flags) -> bool { return ImGui::InputText(label, buffer, size, flags); }

Openable::~Openable() noexcept {
	if (m_open || m_force_close) {
		assert(m_close);
		(*m_close)();
	}
}

Window::Window(char const* label, bool* open_if, int flags) : Canvas(ImGui::Begin(label, open_if, flags), &ImGui::End, true) {}

Window::Window(NotClosed<Canvas>, char const* label, glm::vec2 size, bool border, int flags)
	: Canvas(ImGui::BeginChild(label, {size.x, size.y}, border, flags), &ImGui::EndChild, true) {}

TreeNode::TreeNode(char const* label, int flags) : Openable(ImGui::TreeNodeEx(label, flags), &ImGui::TreePop) {}

auto TreeNode::leaf(char const* label, int flags) -> bool {
	auto tn = TreeNode{label, flags | ImGuiTreeNodeFlags_Leaf};
	return ImGui::IsItemClicked();
}

Window::Menu::Menu(NotClosed<Canvas>) : MenuBar(ImGui::BeginMenuBar(), &ImGui::EndMenuBar) {}

MainMenu::MainMenu() : MenuBar(ImGui::BeginMainMenuBar(), &ImGui::EndMainMenuBar) {}

Popup::Popup(char const* id, bool modal, bool closeable, bool centered, int flags) : Canvas(false, &ImGui::EndPopup) {
	if (centered) {
		auto const center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
	}
	if (modal) {
		m_open = ImGui::BeginPopupModal(id, &closeable, flags);
	} else {
		m_open = ImGui::BeginPopup(id, flags);
	}
}

void Popup::open(char const* id) { ImGui::OpenPopup(id); }

void Popup::close_current() { ImGui::CloseCurrentPopup(); }

Menu::Menu(NotClosed<MenuBar>, char const* label, bool enabled) : Openable(ImGui::BeginMenu(label, enabled), &ImGui::EndMenu) {}

void StyleVar::push(int index, glm::vec2 value) {
	ImGui::PushStyleVar(index, {value.x, value.y});
	++m_count;
}

void StyleVar::push(int index, float value) {
	ImGui::PushStyleVar(index, value);
	++m_count;
}

StyleVar::~StyleVar() { ImGui::PopStyleVar(m_count); }

TabBar::TabBar(char const* label, int flags) : Openable(ImGui::BeginTabBar(label, flags), &ImGui::EndTabBar) {}

TabBar::Item::Item(NotClosed<TabBar>, char const* label, bool* open, int flags) : Openable(ImGui::BeginTabItem(label, open, flags), &ImGui::EndTabItem) {}

Combo::Combo(char const* label, char const* preview) : Openable(ImGui::BeginCombo(label, preview), &ImGui::EndCombo) {}

ListBox::ListBox(char const* label, glm::vec2 size) : Openable(ImGui::BeginListBox(label, {size.x, size.y}), &ImGui::EndListBox) {}
} // namespace le::imcpp
