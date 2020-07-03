#pragma once

namespace le
{
class Window;
}

namespace le::input
{
inline bool g_bFire = true;

void init(Window& out_mainWindow);
void deinit();

void fire();
} // namespace le::input
