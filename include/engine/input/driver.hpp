#pragma once
#include <engine/input/frame.hpp>
#include <window/event_queue.hpp>

namespace le {
struct Viewport;
}

namespace le::input {
using Event = window::Event;

class Driver {
  public:
	using EventQueue = window::EventQueue;
	struct In {
		EventQueue queue;
		struct {
			glm::uvec2 swapchain{};
			glm::vec2 scene{};
		} size;
		f32 renderScale = 1.0f;
		Window const* win{};
	};
	struct Out {
		Frame frame;
		EventQueue residue;
	};

	Out update(In in, Viewport const& view, bool consume = true) noexcept;

  private:
	struct KeySet {
		std::array<KeyMods, 8> keys{};

		bool insert(Key k, Mods const& mods);
		bool erase(Key k) noexcept;
	};

	static void copy(KeySet const& in, ktl::fixed_vector<KeyState, 16>& out_keys, Action action);
	bool extract(Event const& event, State& out_state) noexcept;

	struct {
		ktl::fixed_vector<Gamepad, 8> gamepads;
		ktl::fixed_vector<u32, 4> codepoints;
		KeySet pressed;
		KeySet released;
		KeySet repeated;
	} m_transient;

	struct {
		KeySet held;
		glm::vec2 cursor = {};
		bool suspended = false;
	} m_persistent;
};
} // namespace le::input
