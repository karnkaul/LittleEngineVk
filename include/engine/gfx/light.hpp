#pragma once
#include "core/colour.hpp"
#include "geometry.hpp"

namespace le::gfx
{
struct DirLight final
{
	glm::vec3 direction = -g_nFront;
	Colour ambient = Colour(0x666666ff);
	Colour diffuse = Colour(0xffffffff);
	Colour specular = Colour(0xccccccff);
};
} // namespace le::gfx
