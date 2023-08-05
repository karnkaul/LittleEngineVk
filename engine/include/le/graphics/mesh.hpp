#pragma once
#include <le/core/not_null.hpp>
#include <le/graphics/animation/animation.hpp>
#include <le/graphics/material.hpp>
#include <le/graphics/primitive.hpp>
#include <le/node/node_tree.hpp>

namespace le::graphics {
struct MeshPrimitive {
	NotNull<Primitive const*> primitive;
	Ptr<Material const> material{};
};

struct Skeleton {
	NodeTree joint_tree{};
	std::vector<glm::mat4> inverse_bind_matrices{};
	std::vector<Id<Node>> ordered_joint_ids{};
	std::vector<Ptr<Animation const>> animations{};
};

struct Mesh {
	std::vector<MeshPrimitive> primitives{};
	Ptr<Skeleton const> skeleton{};
};
} // namespace le::graphics
