#pragma once
#include <engine/game/input.hpp>

namespace le
{
class Window;
}

namespace le::input
{
inline bool g_bFire = true;
#if defined(LEVK_EDITOR)
inline bool g_bEditorOnly = false;
#endif

void init(Window& out_mainWindow);
void deinit();

#if defined(LEVK_EDITOR)
[[nodiscard]] Token registerEditorContext(Context const* pContext);
#endif

void fire();
} // namespace le::input
