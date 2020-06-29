#include <unordered_set>
#include <core/flags.hpp>
#include <engine/game/input.hpp>
#include <engine/gfx/camera.hpp>

namespace le
{
class FreeCam : public gfx::Camera
{
public:
	enum class Flag : s8
	{
		eEnabled,
		eKeyToggle_Look,
		eFixedSpeed,
		eTracking,
		eLooking,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	struct KeyToggle final
	{
		input::Key key;
		input::Mods::VALUE mods = input::Mods::eNONE;
		input::Action action = input::Action::eRelease;
	};

	struct Config final
	{
		KeyToggle lookToggle = {input::Key::eL, input::Mods::eCONTROL};
		f32 defaultSpeed = 2.0f;
		f32 minSpeed = 1.0f;
		f32 maxSpeed = 1000.0f;

		f32 mouseLookSens = 0.1f;
		f32 mouseLookEpsilon = 0.2f;
		f32 padLookSens = 50.0f;
		f32 padStickEpsilon = 0.05f;
	};
	struct State final
	{
		std::unordered_set<input::Key> heldKeys;
		std::pair<glm::vec2, glm::vec2> cursorPos = {{0.0f, 0.0f}, {0.0f, 0.0f}};

		f32 speed = 2.0f;
		f32 dSpeed = 0.0f;
		f32 pitch = 0.0f;
		f32 yaw = 0.0f;

		Flags flags;
	};

public:
	State m_state;
	Config m_config;

private:
	input::CtxWrapper m_input;
	glm::vec2 m_dXZ = {};
	glm::vec2 m_dY = {};
	glm::vec2 m_padLook = {};

public:
	void init();
	void tick(Time dt);
	void reset(bool bOrientation, bool bPosition);
};
} // namespace le
