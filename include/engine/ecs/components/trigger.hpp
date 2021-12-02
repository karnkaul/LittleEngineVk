#pragma once
#include <core/std_types.hpp>
#include <dens/entity.hpp>
#include <engine/render/drawable.hpp>
#include <engine/render/material.hpp>
#include <glm/vec3.hpp>
#include <ktl/delegate.hpp>

namespace dens {
class registry;
}

namespace le {
class Transform;

namespace physics {
struct Trigger;
using OnTrigger = ktl::delegate<Trigger const&>;
struct Trigger {
	using CFlags = u32;
	static constexpr CFlags all_flags = 0xffffffff;

	struct Debug;

	glm::vec3 scale = glm::vec3(1.0f);
	glm::vec3 offset = {};
	OnTrigger onTrigger;
	dens::entity entity;
	CFlags cflags = all_flags;

	struct {
		Material material;
	} data;

	void channels(u32 set, u32 unset = 0) noexcept {
		cflags &= ~unset;
		cflags |= set;
	}

	static dens::entity_view<Transform, Trigger> attach(dens::entity entity, dens::registry& out_registry);
};

struct Trigger::Debug {
	std::vector<Drawable> drawables2(dens::registry const& registry) const;
};
} // namespace physics
} // namespace le
