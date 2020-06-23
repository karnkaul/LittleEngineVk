#include <unordered_set>
#include <core/flags.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/window/input_types.hpp>

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
		Key key;
		Mods mods = (Mods)0;
		Action action = Action::eRelease;
	};

	struct Config final
	{
		KeyToggle lookToggle = {Key::eL, Mods::eCONTROL};
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
		std::unordered_set<Key> heldKeys;
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
	OnInput::Token m_tMove;
	OnMouse::Token m_tLook;
	OnMouse::Token m_tZoom;
	OnFocus::Token m_tFocus;
	Window* m_pWindow = nullptr;

public:
	void init(Window* pWindow);
	void tick(Time dt);
	void reset(bool bOrientation, bool bPosition);
};
} // namespace le
