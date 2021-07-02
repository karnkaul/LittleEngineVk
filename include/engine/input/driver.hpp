#pragma once
#include <engine/input/frame.hpp>

namespace le {
struct Viewport;
}

namespace le::input {
class Driver {
  public:
	struct Out {
		Frame frame;
		EventQueue residue;
	};

	Out update(EventQueue queue, Viewport const& view, glm::vec2 size, f32 rscale, bool consume = true, Desktop const* pDI = {}) noexcept;

  private:
	struct KeySet {
		std::array<KeyMods, 8> keys{};

		bool insert(Key k, Mods const& mods);
		bool erase(Key k) noexcept;
	};

	static void copy(KeySet const& in, kt::fixed_vector<KeyAct, 16>& out_keys, Action action);
	bool extract(Event const& event, State& out_state) noexcept;

	struct {
		kt::fixed_vector<Gamepad, 8> gamepads;
		kt::fixed_vector<Event::Cursor, 8> others;
		kt::fixed_vector<char, 4> text;
		KeySet pressed;
		KeySet released;
	} m_transient;

	struct {
		KeySet held;
		glm::vec2 cursor = {};
		bool suspended = false;
	} m_persistent;
};
} // namespace le::input
