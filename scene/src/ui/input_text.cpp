#include <le/core/fixed_string.hpp>
#include <le/engine.hpp>
#include <le/scene/ui/input_text.hpp>

namespace le::ui {
auto InputText::setup() -> void {
	View::setup();

	m_text = &push_element<Text>();
	m_cursor = &push_element<PrimitiveRenderer>();
}

auto InputText::tick(Duration dt) -> void {
	View::tick(dt);
	update_cursor(dt);
}

auto InputText::reset_blink() -> void { m_cursor_elapsed = {}; }

auto InputText::write(std::string_view const str) -> void {
	if (str.empty()) { return; }

	auto text_str = std::string{m_text->get_text()};
	if (cursor.position >= text_str.size()) {
		text_str += str;
	} else {
		text_str.insert(cursor.position, str);
	}

	cursor.position += str.size();
	m_text->set_text(std::move(text_str));
	reset_blink();
}

auto InputText::backspace() -> void {
	if (cursor.position == 0) { return; }

	auto str = m_text->get_text();
	if (str.empty()) { return; }

	if (cursor.position >= str.size()) {
		m_text->set_text(std::string{str.substr(0, str.size() - 1)});
		cursor.position = m_text->get_text().size();
		reset_blink();
		return;
	}

	auto spliced = std::string{str};
	spliced.erase(cursor.position, 1);
	m_text->set_text(std::move(spliced));
	--cursor.position;
	reset_blink();
}

auto InputText::delete_front() -> void { // NOLINT
	auto str = m_text->get_text();

	if (cursor.position >= str.size()) { return; }

	auto spliced = std::string{str};
	spliced.erase(cursor.position, 1);
	m_text->set_text(std::move(spliced));
	reset_blink();
}

auto InputText::step_front() -> void {
	auto const str = m_text->get_text();
	if (cursor.position >= str.size()) { return; }
	++cursor.position;
	reset_blink();
}

auto InputText::step_back() -> void {
	if (cursor.position == 0) { return; }
	--cursor.position;
	reset_blink();
}

auto InputText::goto_home() -> void {
	if (cursor.position == 0) { return; }
	cursor.position = 0;
	reset_blink();
}

auto InputText::goto_end() -> void {
	auto const str = m_text->get_text();
	if (cursor.position >= str.size()) { return; }
	cursor.position = str.size();
	reset_blink();
}

auto InputText::paste_clipboard() -> void {
	if (auto const* str = glfwGetClipboardString(Engine::self().get_window())) { write(str); }
}

auto InputText::on_key(int key, int action, int mods) -> bool {
	if (!enabled) { return false; }

	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_BACKSPACE: backspace(); break;
		case GLFW_KEY_DELETE: delete_front(); break;
		case GLFW_KEY_LEFT: step_back(); break;
		case GLFW_KEY_RIGHT: step_front(); break;
		case GLFW_KEY_HOME: goto_home(); break;
		case GLFW_KEY_END: goto_end(); break;
		case GLFW_KEY_V: {
			if (mods == GLFW_MOD_CONTROL) { paste_clipboard(); }
			break;
		}
		default: break;
		}
	}

	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE: enabled = false; break;
		default: break;
		}
	}

	return true;
}

auto InputText::on_char(input::Codepoint const codepoint) -> bool {
	if (!enabled) { return false; }

	if (codepoint < graphics::Codepoint::eAsciiFirst || codepoint > graphics::Codepoint::eAsciiLast) { return false; }

	auto const ch = static_cast<char>(codepoint);
	write({&ch, 1});

	return true;
}

auto InputText::update_cursor(Duration dt) -> void {
	if (cursor.blink_rate < 0s) { cursor.blink_rate = Cursor::blink_rate_v; }
	cursor.position = std::min(cursor.position, m_text->get_text().size());

	m_cursor_elapsed += dt;
	if (m_cursor_elapsed > cursor.blink_rate) { m_cursor_elapsed = {}; }

	if (enabled) {
		auto const blink_ratio = m_cursor_elapsed / cursor.blink_rate;
		auto const alpha = 2.0f * std::abs(blink_ratio - 0.5f);
		auto tint = m_text->get_tint();
		tint.channels[3] = graphics::Rgba::to_u8(alpha);
		m_cursor->set_tint(tint);
	} else {
		m_cursor->set_tint(graphics::blank_v);
	}

	auto const cursor_size = static_cast<float>(m_text->get_height()) * cursor.scale;
	auto const pre_cursor_text = m_text->get_text().substr(0, cursor.position);
	auto const pre_cursor_text_extent = graphics::Font::Pen{m_text->get_font(), m_text->get_height()}.calc_line_extent(pre_cursor_text);
	auto cursor_offset = m_text->get_text_start() + glm::vec2{pre_cursor_text_extent.x, cursor.n_y_offset * cursor_size.y};
	m_cursor->primitive.set_geometry(graphics::Geometry::from(graphics::Quad{.size = cursor_size, .origin = {cursor_offset, 0.0f}}));
}
} // namespace le::ui
