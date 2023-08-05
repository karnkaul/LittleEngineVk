#include <le/core/zip_ranges.hpp>
#include <le/scene/collision.hpp>
#include <le/scene/scene.hpp>

namespace le {
auto ColliderAabb::aabb() const -> Aabb {
	auto ret = Aabb{.size = aabb_size};
	ret.origin = get_entity().global_position();
	return ret;
}

auto ColliderAabb::setup() -> void { get_scene().collision.track(get_entity()); }

auto Collision::track(Entity const& entity) -> void { m_entries.emplace(entity.id(), Entry{}); }

auto Collision::setup() -> void {
	auto cube = graphics::Geometry::from(graphics::Cube{.size = glm::vec3{1.0f}});
	// clang-format off
	cube.indices = {
		0,	1,	1,	2,	2,	3,	3,	0,
		4,	5,	5,	6,	6,	7,	7,	4,
		8,	9,	9,	10, 10, 11, 11, 8,
		12, 13, 13, 14, 14, 15, 15, 12,
		16, 17, 17, 18, 18, 19, 19, 16,
		20, 21, 21, 22, 22, 23, 23, 20,
	};
	// clang-format on

	m_primitive.set_geometry(cube);
}

auto Collision::tick(Scene const& scene, Duration dt) -> void {
	auto colliders = std::vector<Ptr<Entry>>{};
	for (auto it = m_entries.begin(); it != m_entries.end();) {
		auto const* entity = scene.find_entity(it->first);
		if (entity == nullptr) {
			it = m_entries.erase(it);
			continue;
		}
		auto& entry = it->second;
		entry.collider = entity->find_component<ColliderAabb>();
		if (entry.collider == nullptr) {
			it = m_entries.erase(it);
			continue;
		}
		entry.active = entity->is_active();
		if (!entry.active) {
			++it;
			continue;
		}
		entry.colliding = false;
		entry.aabb = entry.collider->aabb();
		colliders.push_back(&entry);
		++it;
	}
	if (colliders.empty()) { return; }

	auto integrate = [&](Entry& out_a, Entry& out_b) {
		if (Aabb::intersects(out_a.aabb, out_b.aabb)) { return true; }
		if (!out_a.previous_position) { return false; }
		if (out_a.aabb.origin != *out_a.previous_position) {
			for (auto t = Duration{}; t < dt; t += time_slice) {
				auto const position_a = glm::mix(*out_a.previous_position, out_a.aabb.origin, t / dt);
				if (Aabb::intersects(Aabb{position_a, out_a.aabb.size}, out_b.aabb)) { return true; }
			}
		}
		return false;
	};
	for (auto it_a = colliders.begin(); it_a + 1 != colliders.end(); ++it_a) {
		auto& a = **it_a;
		if (!a.active) { continue; }
		for (auto it_b = it_a + 1; it_b != colliders.end(); ++it_b) {
			auto& b = **it_b;
			if (!b.active) { continue; }
			auto const ignore = a.collider->ignore_channels && b.collider->ignore_channels && (a.collider->ignore_channels & b.collider->ignore_channels);
			if (ignore) { continue; }
			if (integrate(a, b)) {
				if (a.collider->on_collision) { a.collider->on_collision(*b.collider); }
				if (b.collider->on_collision) { b.collider->on_collision(*a.collider); }
				a.colliding = b.colliding = true;
			}
		}
		a.previous_position = a.aabb.origin;
	}
}

auto Collision::render_to(std::vector<graphics::RenderObject>& out) const -> void {
	if (draw_line_width < 1.0f || m_entries.empty()) { return; }

	m_render_instances.clear();
	m_render_instances.reserve(m_entries.size());

	for (auto const& [_, entry] : m_entries) {
		if (!entry.active) { continue; }

		auto instance = graphics::RenderInstance{.tint = entry.colliding ? graphics::red_v : graphics::green_v};
		instance.transform.set_position(entry.aabb.origin);
		instance.transform.set_scale(entry.aabb.size);
		m_render_instances.push_back(instance);
	}

	out.push_back(graphics::RenderObject{
		.material = &m_material,
		.primitive = &m_primitive,
		.instances = m_render_instances,
		.pipeline_state = m_pipeline,
	});
}

auto Collision::clear() -> void { m_entries.clear(); }
} // namespace le
