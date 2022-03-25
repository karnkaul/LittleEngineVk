#pragma once
#include <ktl/kunique_ptr.hpp>
#include <levk/engine/input/frame.hpp>
#include <levk/window/event.hpp>

namespace le {
struct Viewport;
}

namespace le::input {
using Event = window::Event;
using EventQueue = std::vector<Event>;

struct EventParser {
	Opt<EventParser> next{};
	virtual bool operator()(Event const& event) = 0;
};

class Driver {
  public:
	struct In {
		Span<Event const> events;
		struct {
			glm::uvec2 swapchain{};
			glm::vec2 scene{};
		} size;
		f32 renderScale = 1.0f;
		Window const* win{};
		EventParser* customParser{};
	};

	static void parse(Span<Event const> events, EventParser& parser);

	Driver();

	Frame update(In const& in, Viewport const& view);

  private:
	struct Parser;

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
		bool iconified = false;
	} m_persistent;

	KeyQueue m_keyQueue;
	ktl::kunique_ptr<KeyDB> m_keyDB; // 4K bytes, hence on heap
};
} // namespace le::input
