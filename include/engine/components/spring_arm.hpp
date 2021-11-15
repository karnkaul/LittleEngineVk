#pragma once
#include <core/std_types.hpp>
#include <core/time.hpp>
#include <dens/entity.hpp>
#include <engine/editor/inspect.hpp>
#include <glm/vec3.hpp>

namespace dens {
class registry;
}

namespace le {
class Transform;

struct SpringArm {
	dens::entity target;
	glm::vec3 offset = {};
	Time_s fixed = 2ms;
	f32 k = 0.5f;
	f32 b = 0.05f;
	struct {
		glm::vec3 velocity = {};
		Time_s ft;
	} data;

	static dens::entity_view<SpringArm, Transform> attach(dens::entity entity, dens::registry& out, dens::entity target = {});
	static dens::entity_view<SpringArm, Transform> make(dens::registry& out, dens::entity target = {});

	static void inspect(edi::Inspect<SpringArm> out);
};
} // namespace le
