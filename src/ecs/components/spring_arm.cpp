#include <core/transform.hpp>
#include <dens/registry.hpp>
#include <engine/ecs/components/spring_arm.hpp>
#include <engine/editor/types.hpp>

namespace le {
dens::entity_view<SpringArm, Transform> SpringArm::attach(dens::entity entity, dens::registry& out, dens::entity target) {
	if (!out.attached<Transform>(entity)) { out.attach<Transform>(entity); }
	out.attach<SpringArm>(entity).target = target;
	auto ret = std::tie(out.get<SpringArm>(entity), out.get<Transform>(entity));
	return {entity, ret};
}

dens::entity_view<SpringArm, Transform> SpringArm::make(dens::registry& out, dens::entity target) { return attach(out.make_entity(), out, target); }

void SpringArm::inspect(edi::Inspect<SpringArm> inspect) {
	auto& spring = inspect.get();
	edi::TWidget<f32>("k##springarm", spring.k, 0.01f);
	edi::TWidget<f32>("b##springarm", spring.b, 0.001f);
	edi::TWidget<glm::vec3>("offset##springarm", spring.offset, false);
	auto const name = inspect.registry.contains(spring.target) ? inspect.registry.name(spring.target) : "[None]";
	auto const label = ktl::stack_string<128>("%s [%u]", name.data(), inspect.entity.id);
	edi::TWidget<std::string_view> select("target##springarm", label);
	if (auto target = edi::DragDrop::Target()) {
		if (auto e = target.payload<dens::entity>("ENTITY")) { spring.target = *e; }
	}
}
} // namespace le
