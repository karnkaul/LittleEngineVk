#pragma once
#include <glm/vec2.hpp>
#include <levk/core/polymorphic.hpp>
#include <cstdint>

namespace levk {
/// \brief Abstract base class for Event consumers.
class IEventSink : public Polymorphic {
  public:
	virtual void on_focus(bool in_focus) = 0;
	virtual auto on_close() -> bool = 0;
	virtual void on_window_resize(glm::vec2 window_size) = 0;
	virtual void on_framebuffer_resize(glm::vec2 framebuffer_size) = 0;
	virtual void on_key_down(int key, int mods) = 0;
	virtual void on_key_up(int key, int mods) = 0;
	virtual void on_key_repeat(int key, int mods) = 0;
	virtual void on_char(std::uint32_t code) = 0;
	virtual void on_mouse_down(glm::ivec2 position, int button, int mods) = 0;
	virtual void on_mouse_up(glm::ivec2 position, int button, int mods) = 0;
	virtual void on_cursor_move(glm::ivec2 position) = 0;
	virtual void on_scroll_wheel(glm::vec2 scroll) = 0;
	virtual void on_file_drop(std::span<char const*> paths) = 0;
};

/// \brief Concrete base class for Event consumers.
class EventSink : public IEventSink {
  public:
	void on_focus([[maybe_unused]] bool in_focus) override {}
	auto on_close() -> bool override { return true; }
	void on_window_resize([[maybe_unused]] glm::vec2 window_size) override {}
	void on_framebuffer_resize([[maybe_unused]] glm::vec2 framebuffer_size) override {}
	void on_key_down([[maybe_unused]] int key, [[maybe_unused]] int mods) override {}
	void on_key_up([[maybe_unused]] int key, [[maybe_unused]] int mods) override {}
	void on_key_repeat([[maybe_unused]] int key, [[maybe_unused]] int mods) override {}
	void on_char([[maybe_unused]] std::uint32_t code) override {}
	void on_mouse_down([[maybe_unused]] glm::ivec2 position, [[maybe_unused]] int button, [[maybe_unused]] int mods) override {}
	void on_mouse_up([[maybe_unused]] glm::ivec2 position, [[maybe_unused]] int button, [[maybe_unused]] int mods) override {}
	void on_cursor_move([[maybe_unused]] glm::ivec2 position) override {}
	void on_scroll_wheel([[maybe_unused]] glm::vec2 scroll) override {}
	void on_file_drop([[maybe_unused]] std::span<char const*> paths) override {}
};
} // namespace levk
