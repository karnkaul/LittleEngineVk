#pragma once
#include <engine/gfx/renderer.hpp>
#include <engine/levk.hpp>

namespace le::gs
{
gfx::Renderer::Scene update(engine::Driver& out_driver, Time dt, bool bTick);

#if defined(LEVK_EDITOR)
using EMap = std::unordered_map<Transform*, Entity>;
EMap& entityMap();
Transform& root();
#endif
} // namespace le::gs
