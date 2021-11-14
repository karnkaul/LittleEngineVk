#include <core/transform.hpp>
#include <dens/registry.hpp>
#include <engine/components/spring_arm.hpp>
#include <engine/editor/types.hpp>

namespace le {
dens::entity_view<SpringArm, Transform> SpringArm::attach(dens::entity entity, dens::registry& out, dens::entity target) {
	out.attach<SpringArm, Transform>(entity);
	auto ret = std::tie(out.get<SpringArm>(entity), out.get<Transform>(entity));
	std::get<SpringArm&>(ret).target = target;
	return {entity, ret};
}

dens::entity_view<SpringArm, Transform> SpringArm::make(dens::registry& out, dens::entity target) { return attach(out.make_entity(), out, target); }

void SpringArm::inspect(edi::Inspect<SpringArm> inspect) {
	auto& spring = inspect.get();
	edi::TWidget<f32>("k", spring.k, 0.01f);
	edi::TWidget<f32>("b", spring.b, 0.001f);
	edi::TWidget<glm::vec3>("offset", spring.offset, false);
	if (auto tn = edi::TreeNode("target")) {
		edi::Text(inspect.registry.contains(spring.target) ? inspect.registry.name(spring.target) : "[None]");
		edi::Text(ktl::stack_string<8>("id: %u", spring.target.id));
		if (edi::Button("Change")) { edi::Popup::open("change_springarm_target"); }
		int const current = int(spring.target.id);
		static int s_customTarget{};
		if (auto change = edi::Popup("change_springarm_target")) {
			auto replace = dens::entity{std::size_t(s_customTarget), inspect.registry.id()};
			std::string_view name = "[Invalid]";
			bool valid{};
			if (replace.id != inspect.entity.id && inspect.registry.contains(replace)) {
				name = inspect.registry.name(replace);
				valid = true;
			}
			edi::TWidget<int>(ktl::stack_string<128>("%s###springarm_target_id", name.data()), s_customTarget, 100.0f, {}, edi::WType::eInput);
			if (valid && edi::Button("Set##springarm_target")) {
				if (inspect.registry.contains(replace)) { spring.target = replace; }
			}
		} else {
			s_customTarget = current;
		}
	}
}
} // namespace le
