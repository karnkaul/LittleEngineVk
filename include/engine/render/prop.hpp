#pragma once

// TODO: remove
#include <engine/render/material.hpp>

namespace le {
namespace graphics {
class MeshPrimitive;
} // namespace graphics
struct Material;

struct Prop {
	Material const* material = {};
	graphics::MeshPrimitive const* primitive = {};
};
} // namespace le
