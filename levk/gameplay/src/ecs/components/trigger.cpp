#include <dens/registry.hpp>
#include <levk/core/transform.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/gameplay/ecs/components/trigger.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/render/draw_list.hpp>

namespace le::physics {
dens::entity_view<Transform, Trigger> Trigger::attach(dens::entity entity, dens::registry& out) {
	if (!out.attached<Transform>(entity)) { out.attach<Transform>(entity); }
	out.attach<Trigger>(entity);
	auto ret = std::tie(out.get<Transform>(entity), out.get<Trigger>(entity));
	return {entity, ret};
}

bool Trigger::Debug::addDrawPrimitives(AssetStore const& store, dens::registry const& registry, graphics::DrawList& out) const {
	if (auto mesh = store.find<graphics::MeshPrimitive>("wireframes/cube")) {
		for (auto const& [e, c] : registry.view<Trigger, Transform>()) {
			auto& [trigger, transform] = c;
			static constexpr auto idty = glm::mat4(1.0f);
			auto const matrix = glm::translate(idty, transform.position() + trigger.offset) * glm::scale(idty, trigger.scale);
			out.push(graphics::DrawPrimitive{{}, mesh, &trigger.data.material}, matrix);
		}
		return true;
	}
	return false;
}
} // namespace le::physics
