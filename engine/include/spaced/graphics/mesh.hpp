#pragma once
#include <spaced/core/not_null.hpp>
#include <spaced/graphics/animation/animation.hpp>
#include <spaced/graphics/material.hpp>
#include <spaced/graphics/primitive.hpp>
#include <spaced/node/node_tree.hpp>

namespace spaced::graphics {
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
} // namespace spaced::graphics
