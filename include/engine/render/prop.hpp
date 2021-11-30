#pragma once
#include <engine/render/material.hpp>

namespace le {
namespace graphics {
class MeshPrimitive;
} // namespace graphics

struct Prop {
	Material material;
	graphics::MeshPrimitive const* mesh = {};
};
} // namespace le
