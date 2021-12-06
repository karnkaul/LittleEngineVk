#pragma once
#include <core/std_types.hpp>
#include <engine/scene/mesh_view.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <ktl/fixed_vector.hpp>

namespace le {
class DescriptorUpdater;
class DescriptorMap;

struct DrawScissor {
	glm::uvec2 extent{};
	glm::ivec2 offset{};
	bool set{};
};

struct DrawBindable {
	static constexpr std::size_t max_sets_v = 16U;

	mutable ktl::fixed_vector<u32, max_sets_v> sets;

	DescriptorUpdater set(DescriptorMap& map, u32 set) const;
};

struct DrawMesh : DrawBindable {
	MeshView mesh;
};

struct Drawable : DrawBindable {
	glm::mat4 model = glm::mat4(1.0f);
	DrawMesh mesh;
	DrawScissor scissor;

	MeshObjView meshViews() const { return mesh.mesh.meshViews(); }
};

struct NoDraw {};
} // namespace le
