#pragma once
#include <glm/vec2.hpp>
#include <levk/core/polymorphic.hpp>

namespace levk::input {
/// \brief Abstract base class for input receivers.
class IReceiver : public Polymorphic {
  public:
	[[nodiscard]] virtual auto is_blocking() const -> bool = 0;

	virtual void on_focus(bool in_focus) = 0;
	virtual void on_key_down(int key, int mods) = 0;
	virtual void on_key_up(int key, int mods) = 0;
	virtual void on_key_repeat(int key, int mods) = 0;
	virtual void on_mouse_down(glm::ivec2 position, int button, int mods) = 0;
	virtual void on_mouse_up(glm::ivec2 position, int button, int mods) = 0;
	virtual void on_cursor_move(glm::ivec2 position) = 0;
	virtual void on_scroll_wheel(glm::vec2 scroll) = 0;
};

/// \brief Concrete base class for input receivers.
class Receiver : public IReceiver {
  public:
	[[nodiscard]] auto is_blocking() const -> bool override { return false; }

	void on_focus([[maybe_unused]] bool in_focus) override {}
	void on_key_down([[maybe_unused]] int key, [[maybe_unused]] int mods) override {}
	void on_key_up([[maybe_unused]] int key, [[maybe_unused]] int mods) override {}
	void on_key_repeat([[maybe_unused]] int key, [[maybe_unused]] int mods) override {}
	void on_mouse_down([[maybe_unused]] glm::ivec2 position, [[maybe_unused]] int button, [[maybe_unused]] int mods) override {}
	void on_mouse_up([[maybe_unused]] glm::ivec2 position, [[maybe_unused]] int button, [[maybe_unused]] int mods) override {}
	void on_cursor_move([[maybe_unused]] glm::ivec2 position) override {}
	void on_scroll_wheel([[maybe_unused]] glm::vec2 scroll) override {}
};
} // namespace levk::input
