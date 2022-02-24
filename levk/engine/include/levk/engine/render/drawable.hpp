#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <ktl/fixed_vector.hpp>
#include <levk/core/std_types.hpp>
#include <levk/engine/render/mesh_view.hpp>
#include <levk/graphics/mesh_view.hpp>
#include <optional>

namespace le {
class DescriptorUpdater;
class DescriptorMap;

struct DrawScissor {
	glm::uvec2 extent{};
	glm::ivec2 offset{};
};

struct DrawBindable {
	static constexpr std::size_t max_sets_v = 16U;

	mutable ktl::fixed_vector<u32, max_sets_v> sets;

	DescriptorUpdater set(DescriptorMap& map, u32 set) const;
};

struct DrawMesh : DrawBindable {
	MeshView mesh;
	std::vector<graphics::PrimitiveView> mesh2{};
};

struct Drawable : DrawBindable {
	glm::mat4 model = glm::mat4(1.0f);
	DrawMesh mesh;
	std::optional<DrawScissor> scissor;

	MeshObjView meshViews() const { return mesh.mesh.meshViews(); }
};

struct Drawable2 {
	glm::mat4 matrix = glm::mat4(1.0f);
	std::optional<DrawScissor> scissor;
	std::vector<graphics::PrimitiveView> primitives;
};

struct NoDraw {};
} // namespace le
