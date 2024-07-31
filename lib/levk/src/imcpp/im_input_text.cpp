#include <levk/core/ptr.hpp>
#include <levk/imcpp/im_input_text.hpp>
#include <cassert>
#include <cstring>

namespace levk {
auto ImInputText::update(CString const name) -> bool {
	if (m_buffer.empty()) { m_buffer.resize(init_size_v, '\0'); }
	auto const cb = [](Ptr<ImGuiInputTextCallbackData> data) { return static_cast<ImInputText*>(data->UserData)->on_callback(*data); };
	return ImGui::InputText(name.c_str(), m_buffer.data(), m_buffer.size(), ImGuiInputTextFlags_CallbackResize | flags, cb, this);
}

void ImInputText::set_text(std::string_view const text) {
	if (text.empty()) {
		m_buffer.clear();
		return;
	}
	m_buffer.resize(text.size() + 1, '\0');
	std::memcpy(m_buffer.data(), text.data(), text.size());
	m_buffer.at(text.size()) = '\0';
}

auto ImInputText::on_callback(ImGuiInputTextCallbackData& data) -> int {
	switch (data.EventFlag) {
	case ImGuiInputTextFlags_CallbackResize: resize_buffer(data); break;
	default: break;
	}
	return 0;
}

void ImInputText::resize_buffer(ImGuiInputTextCallbackData& data) {
	assert(!m_buffer.empty());
	m_buffer.resize(m_buffer.size() * 2);
	data.BufSize = static_cast<int>(m_buffer.size());
	data.Buf = m_buffer.data();
	data.BufDirty = true;
}

auto ImInputTextMultiLine::update(CString const name) -> bool {
	if (m_buffer.empty()) { m_buffer.resize(init_size_v, '\0'); }
	auto const cb = [](Ptr<ImGuiInputTextCallbackData> data) { return static_cast<ImInputTextMultiLine*>(data->UserData)->on_callback(*data); };
	return ImGui::InputTextMultiline(name.c_str(), m_buffer.data(), m_buffer.size(), size, ImGuiInputTextFlags_CallbackResize | flags, cb, this);
}
} // namespace levk
