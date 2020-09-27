#pragma once
#if defined(LEVK_EDITOR)
#include <glm/glm.hpp>
#include <core/time.hpp>
#include <core/trigger.hpp>
#include <engine/gfx/screen_rect.hpp>
#include <engine/window/common.hpp>
#include <engine/game/freecam.hpp>
#include <engine/game/state.hpp>
#include <game/state_impl.hpp>

namespace le::editor
{
inline bool g_bTickGame = true;
inline TTrigger<bool> g_bStepGame = false;
inline gfx::ScreenRect g_gameRect = {};
inline FreeCam g_editorCam;

bool init(WindowID editorWindow);
void deinit();
void tick(GameScene& out_scene, Time dt);
} // namespace le::editor
#endif
