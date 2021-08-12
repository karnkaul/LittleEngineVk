#pragma once
#include <engine/render/material.hpp>

namespace le {
namespace graphics {
class Mesh;
} // namespace graphics

struct Prop {
	Material material;
	graphics::Mesh const* mesh = {};
};
} // namespace le
