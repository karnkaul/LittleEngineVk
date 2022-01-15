#pragma once
#include <levk/engine/input/frame.hpp>
#include <levk/window/event.hpp>
#include <memory>

namespace le {
struct Viewport;
}

namespace le::input {
using Event = window::Event;
using EventQueue = std::vector<Event>;

// Calls parser(Event const&, State&) and expects it to return true on consuming event
template <typename Parser>
EventQueue parse(Parser& parser, Span<Event const> in, State& out);

class Driver {
  public:
	struct In {
		std::vector<Event> queue;
		struct {
			glm::uvec2 swapchain{};
			glm::vec2 scene{};
		} size;
		f32 renderScale = 1.0f;
		Window const* win{};
	};
	struct Out {
		Frame frame;
		std::vector<Event> residue;
	};

	Driver();

	Out update(In in, Viewport const& view, bool consume = true);

	bool operator()(Event const& event, State& out_state);

  private:
	struct KeyQueue {
		ktl::fixed_vector<KeyEvent, 16> keys;

		void insert(KeyEvent event) noexcept;
		void collapse(KeyDB& out) noexcept;
	};

	struct {
		ktl::fixed_vector<Gamepad, 8> gamepads;
		ktl::fixed_vector<u32, 4> codepoints;
	} m_transient;

	struct {
		glm::vec2 cursor = {};
		bool suspended = false;
	} m_persistent;

	KeyQueue m_keyQueue;
	std::unique_ptr<KeyDB> m_keyDB; // 4K bytes, hence on heap
};

// impl

template <typename Parser>
std::vector<Event> parse(Parser& parser, Span<Event const> in, State& out) {
	std::vector<Event> ret;
	for (auto const& event : in) {
		if (!parser(event, out)) { ret.push_back(event); }
	}
	return ret;
}
} // namespace le::input
