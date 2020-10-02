#pragma once
#include <unordered_set>
#include <engine/game/input.hpp>
#include <engine/gfx/camera.hpp>
#include <kt/enum_flags/enum_flags.hpp>

namespace le {
class FreeCam {
  public:
	enum class Flag : s8 { eEnabled, eKeyToggle_Look, eFixedSpeed, eTracking, eLooking, eKeyLook, eCOUNT_ };
	using Flags = kt::enum_flags<Flag>;

	struct KeyToggle final {
		input::Key key;
		input::Mods::VALUE mods = input::Mods::eNONE;
		input::Action action = input::Action::eRelease;
	};

	struct Config final {
		KeyToggle lookToggle = {input::Key::eL};
		f32 defaultSpeed = 2.0f;
		f32 minSpeed = 1.0f;
		f32 maxSpeed = 1000.0f;

		f32 mouseLookSens = 0.1f;
		f32 mouseLookEpsilon = 0.2f;
		f32 padLookSens = 50.0f;
		f32 padStickEpsilon = 0.05f;
	};
	struct State final {
		std::unordered_set<input::Key> heldKeys;
		std::pair<glm::vec2, glm::vec2> cursorPos = {{0.0f, 0.0f}, {0.0f, 0.0f}};

		f32 speed = 2.0f;
		f32 dSpeed = 0.0f;
		f32 pitch = 0.0f;
		f32 yaw = 0.0f;

		Flags flags;
	};

  public:
	gfx::Camera m_camera;
	State m_state;
	Config m_config;

  public:
	void reset();

  public:
#if defined(LEVK_EDITOR)
	void init(bool bEditorContext = false);
#else
	void init();
#endif
	void tick(Time dt);

  private:
	input::Context m_input;
	Token m_token;
	glm::vec2 m_dXZ = {};
	glm::vec2 m_dY = {};
	glm::vec2 m_padLook = {};
};
} // namespace le
