#pragma once
#include <unordered_set>
#include <utility>
#include <core/flags.hpp>
#include <engine/gfx/geometry.hpp>
#include <engine/window/input_types.hpp>

namespace le
{
class Window;
}

namespace le::gfx
{
// Camera faces -Z
class Camera
{
public:
	glm::vec3 m_position = {};
	glm::quat m_orientation = g_qIdentity;
	f32 m_fov = 45.0f;

public:
	Camera();
	Camera(Camera&&);
	Camera& operator=(Camera&&);
	virtual ~Camera();

public:
	virtual void tick(Time dt);

public:
	glm::mat4 view() const;
	glm::mat4 perspective(f32 aspect, f32 near = 0.1f, f32 far = 100.0f) const;
	glm::mat4 ortho(f32 aspect, f32 zoom = 1.0f, f32 near = 0.1f, f32 far = 100.0f) const;
	glm::mat4 ui(glm::vec3 const& uiSpace) const;
};

class FreeCam : public Camera
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
	Window* m_pWindow;

public:
	FreeCam(Window* pWindow);

public:
	void tick(Time dt) override;
	void reset(bool bOrientation, bool bPosition);
};
} // namespace le::gfx
