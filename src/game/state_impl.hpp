#pragma once
#include <engine/gfx/renderer.hpp>
#include <engine/levk.hpp>

namespace le::gs
{
gfx::Renderer::Scene update(engine::Driver& out_driver, Time dt, bool bTick);
} // namespace le::gs
