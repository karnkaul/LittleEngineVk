#pragma once
#if defined(LEVK_EDITOR)
#include <glm/glm.hpp>
#include "engine/gfx/renderer.hpp"

namespace le::editor
{
inline bool g_bTickGame = true;

bool init();
void deinit();
bool render(gfx::ScreenRect const& scene, glm::ivec2 const& fbSize);
} // namespace le::editor
#endif
