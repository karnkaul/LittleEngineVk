#pragma once
#include <levk/input/receiver.hpp>
#include <memory>
#include <ranges>
#include <vector>

namespace levk::input {
class Dispatcher : public IReceiver {
  public:
	void on_focus(bool const in_focus) override {
		on_input_event([in_focus](IReceiver& receiver) { receiver.on_focus(in_focus); });
	}

	void on_key_down(int const key, int const mods) override {
		on_input_event([key, mods](IReceiver& receiver) { receiver.on_key_down(key, mods); });
	}

	void on_key_up(int const key, int const mods) override {
		on_input_event([key, mods](IReceiver& receiver) { receiver.on_key_up(key, mods); });
	}

	void on_key_repeat(int const key, int const mods) override {
		on_input_event([key, mods](IReceiver& receiver) { receiver.on_key_repeat(key, mods); });
	}

	void on_mouse_down(glm::ivec2 const position, int const button, int const mods) override {
		on_input_event([position, button, mods](IReceiver& receiver) { receiver.on_mouse_down(position, button, mods); });
	}

	void on_mouse_up(glm::ivec2 position, int button, int mods) override {
		on_input_event([position, button, mods](IReceiver& receiver) { receiver.on_mouse_up(position, button, mods); });
	}

	void on_cursor_move(glm::ivec2 position) override {
		on_input_event([position](IReceiver& receiver) { receiver.on_cursor_move(position); });
	}

	void on_scroll_wheel(glm::vec2 scroll) override {
		on_input_event([scroll](IReceiver& receiver) { receiver.on_scroll_wheel(scroll); });
	}

	void attach_receiver(std::shared_ptr<input::IReceiver> const& receiver) {
		if (!receiver || receiver.get() == this) { return; }
		m_input_receivers.push_back(receiver);
	}

  protected:
	template <typename Func>
	void on_input_event(Func func) {
		on_event(m_input_receivers, func);
		std::erase_if(m_input_receivers, [](auto const& r) { return r.expired(); });
	}

	std::vector<std::weak_ptr<input::IReceiver>> m_input_receivers{};
};

template <typename Func>
void on_event(std::span<std::weak_ptr<input::IReceiver>> receivers, Func func) {
	for (auto const& w_receiver : std::ranges::reverse_view(receivers)) {
		auto const& receiver = w_receiver.lock();
		if (!receiver) { continue; }
		func(*receiver);
		if (receiver->is_blocking()) { return; }
	}
}
} // namespace levk::input
