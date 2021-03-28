#pragma once
#if defined(LEVK_EDITOR)
#include <core/time.hpp>
#include <core/trigger.hpp>
#include <engine/game/freecam.hpp>
#include <engine/game/state.hpp>
#include <engine/gfx/viewport.hpp>
#include <engine/window/common.hpp>
#include <game/state_impl.hpp>
#include <glm/glm.hpp>

namespace le::editor {
inline bool g_bTickGame = true;
inline TTrigger<bool> g_bStepGame = false;
inline FreeCam_OLD g_editorCam;

bool init(WindowID editorWindow);
bool enabled();
void deinit();
std::optional<gfx::Viewport> tick(GameScene& out_scene, Time_s dt);
} // namespace le::editor
#endif
