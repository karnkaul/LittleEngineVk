#include <core/services.hpp>
#include <core/transform.hpp>
#include <dens/registry.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/ecs/components/trigger.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le::physics {
namespace {
graphics::MeshPrimitive const* cubeMesh() {
	if (auto store = Services::find<AssetStore>()) {
		if (auto mesh = store->find<graphics::MeshPrimitive>("wireframes/cube")) { return &*mesh; }
	}
	return {};
}
} // namespace

dens::entity_view<Transform, Trigger> Trigger::attach(dens::entity entity, dens::registry& out) {
	if (!out.attached<Transform>(entity)) { out.attach<Transform>(entity); }
	out.attach<Trigger>(entity);
	auto ret = std::tie(out.get<Transform>(entity), out.get<Trigger>(entity));
	return {entity, ret};
}

std::vector<Drawable> Trigger::Debug::drawables(dens::registry const& registry) const {
	std::vector<Drawable> ret;
	if (auto mesh = cubeMesh()) {
		for (auto const& [e, c] : registry.view<Trigger, Transform>()) {
			auto& [trigger, transform] = c;
			trigger.data.prop = {&trigger.data.material, mesh};
			Drawable drawable;
			drawable.props = trigger.data.prop;
			static constexpr auto idty = glm::mat4(1.0f);
			drawable.model = glm::translate(idty, transform.position() + trigger.offset) * glm::scale(idty, trigger.scale);
			ret.push_back(drawable);
		}
	}
	return ret;
}

std::vector<Drawable2> Trigger::Debug::drawables2(dens::registry const& registry) const {
	std::vector<Drawable2> ret;
	if (auto mesh = cubeMesh()) {
		for (auto const& [e, c] : registry.view<Trigger, Transform>()) {
			auto& [trigger, transform] = c;
			trigger.data.prop = {&trigger.data.material, mesh};
			Drawable2 drawable;
			drawable.mesh = MeshObj{mesh, &trigger.data.material};
			static constexpr auto idty = glm::mat4(1.0f);
			drawable.model = glm::translate(idty, transform.position() + trigger.offset) * glm::scale(idty, trigger.scale);
			ret.push_back(drawable);
		}
	}
	return ret;
}
} // namespace le::physics
