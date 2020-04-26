#pragma once
#include "geometry.hpp"

namespace le::gfx
{
Geometry createQuad(f32 side = 1.0f);
Geometry createCube(f32 side = 1.0f);
Geometry createCircle(f32 diameter, u16 points);
Geometry createCubedSphere(f32 diameter, u8 quadsPerSide);
} // namespace le::gfx
