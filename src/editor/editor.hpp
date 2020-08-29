#pragma once
#if defined(LEVK_EDITOR)
#include <glm/glm.hpp>
#include <core/ecs_registry.hpp>
#include <core/time.hpp>
#include <engine/gfx/screen_rect.hpp>
#include <engine/window/common.hpp>
#include <engine/game/freecam.hpp>
#include <engine/game/state.hpp>
#include <game/state_impl.hpp>

namespace le::editor
{
inline bool g_bTickGame = true;
inline gfx::ScreenRect g_gameRect = {};
inline FreeCam g_editorCam;

struct Args final
{
	gs::Context* pGame = nullptr;
	gs::EMap* pMap = nullptr;
	Transform* pRoot = nullptr;
};

bool init(WindowID editorWindow);
void deinit();
void tick(Args const& args, Time dt);
} // namespace le::editor
#endif
