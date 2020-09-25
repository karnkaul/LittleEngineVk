#pragma once
#include <engine/gfx/render_driver.hpp>
#include <engine/levk.hpp>

namespace le::gs
{
gfx::render::Driver::Scene update(engine::Driver& out_driver, Time dt, bool bTick);
} // namespace le::gs
