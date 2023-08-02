#pragma once
#include <spaced/build_version.hpp>
#include <spaced/node/node_tree.hpp>
#include <spaced/scene/entity.hpp>
#include <functional>
#include <optional>
#include <unordered_map>

namespace spaced {
struct Aabb {
	glm::vec3 origin{};
	glm::vec3 size{1.0f};

	[[nodiscard]] constexpr auto contains(glm::vec3 const point) const -> bool {
		if (size == glm::vec3{}) { return false; }
		auto const hs = 0.5f * size;
		constexpr auto test = [](float o, float h, float p) { return p >= o - h && p <= o + h; };
		if (!test(origin.x, hs.x, point.x)) { return false; }
		if (!test(origin.y, hs.y, point.y)) { return false; }
		if (!test(origin.z, hs.z, point.z)) { return false; }
		return true;
	}

	[[nodiscard]] constexpr auto contains(Aabb const& rhs) const -> bool {
		if (size == glm::vec3{} || rhs.size == glm::vec3{}) { return false; }
		auto const hs = 0.5f * size;
		auto const hs_r = 0.5f * rhs.size;
		if (origin.x - hs.x > rhs.origin.x + hs_r.x) { return false; }
		if (origin.x + hs.x < rhs.origin.x - hs_r.x) { return false; }
		if (origin.y - hs.y > rhs.origin.y + hs_r.y) { return false; }
		if (origin.y + hs.y < rhs.origin.y - hs_r.y) { return false; }
		if (origin.z - hs.z > rhs.origin.z + hs_r.z) { return false; }
		if (origin.z + hs.z < rhs.origin.z - hs_r.z) { return false; }
		return true;
	}

	[[nodiscard]] static constexpr auto intersects(Aabb const& a, Aabb const& b) -> bool { return a.contains(b); }

	auto operator==(Aabb const&) const -> bool = default;
};

class ColliderAabb : public Component {
  public:
	[[nodiscard]] auto aabb() const -> Aabb;

	auto setup() -> void override;
	auto tick(Duration /*dt*/) -> void override {}

	glm::vec3 aabb_size{1.0f};
	std::function<void(ColliderAabb const&)> on_collision{};
	std::uint32_t ignore_channels{};
};

class Collision {
  public:
	static constexpr auto draw_line_width_v{3.0f};

	Collision(Collision const&) = delete;
	Collision(Collision&&) = delete;
	auto operator=(Collision&&) -> Collision& = delete;
	auto operator=(Collision const&) -> Collision& = delete;

	Collision() = default;
	~Collision() = default;

	auto track(Entity const& entity) -> void;

	auto setup() -> void;
	auto tick(Scene const& scene, Duration dt) -> void;
	auto render_to(std::vector<graphics::RenderObject>& out) const -> void;

	auto clear() -> void;

	Duration time_slice{5ms};
	float draw_line_width{debug_v ? draw_line_width_v : 0.0f};

  protected:
	struct Entry {
		Ptr<ColliderAabb> collider{};
		Aabb aabb{};
		std::optional<glm::vec3> previous_position{};
		bool colliding{};
		bool active{};
	};
	using Map = std::unordered_map<Id<Entity>::id_type, Entry>;

	Map m_entries{};

	graphics::UnlitMaterial m_material{};
	graphics::PipelineState m_pipeline{
		.topology = vk::PrimitiveTopology::eLineList,
		.depth_test_write = 0,
		.line_width = 3.0f,
	};
	graphics::StaticPrimitive m_primitive{};

	mutable std::vector<graphics::RenderInstance> m_render_instances{};
};
} // namespace spaced
