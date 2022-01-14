#pragma once
#include <core/nvec.hpp>
#include <core/time.hpp>
#include <core/transform.hpp>
#include <dumb_tasks/task_queue.hpp>
#include <levk/engine/input/space.hpp>
#include <levk/graphics/geometry.hpp>
#include <levk/graphics/render/rgba.hpp>
#include <utility>
#include <vector>

namespace le {
struct EmitterInfo {
	static constexpr glm::vec3 xy = glm::vec3(1.0f, 1.0f, 0.0f);

	template <typename T>
	using Range = std::pair<T, T>;
	using RGBA = graphics::RGBA;

	struct {
		Range<glm::vec3> position = {};
		Range<f32> scale = {1.0f, 1.0f};
		Range<f32> alpha = {1.0f, 1.0f};
		struct {
			Range<glm::vec3> direction = {-xy, xy};
			Range<glm::vec3> speed = {100.0f * xy, 200.0f * xy};
		} linear;
		struct {
			Range<f32> speed = {-180.0f, 180.0f};
		} angular;
		Range<Time_s> ttl = {2s, 5s};
	} init;
	struct {
		f32 alpha = 0.0f;
		f32 scale = 0.0f;
	} end;

	glm::vec2 size = {100.0f, 100.0f};
	RGBA colour = colours::white;
	std::size_t count = 100;
	bool loop = true;
};

class QuadEmitter {
  public:
	void create(EmitterInfo const& info);
	void tick(Time_s dt, Opt<dts::task_queue> tasks = {});
	graphics::Geometry geometry() const;

  private:
	void addQuad();

	template <typename T>
	struct VelPos {
		T position{};
		T velocity{};

		void tick(Time_s dt) noexcept { position += velocity * dt.count(); }
	};

	struct Pos;
	struct Geom;

	EmitterInfo m_info;
	struct {
		std::vector<Pos> pos;
		std::vector<Geom> gen;
		std::vector<dts::task_id> tids;
	} m_data;
};

struct QuadEmitter::Pos {
	VelPos<glm::vec3> linear;
	VelPos<f32> radial;
	Time_s ttl;

	Time_s elapsed{};
	f32 ratio = 0.0f;

	void operator()(Time_s dt) noexcept;
};

struct QuadEmitter::Geom {
	graphics::Geometry geometry;
	EmitterInfo::Range<f32> scale;
	EmitterInfo::Range<f32> alpha;

	void operator()(glm::vec3 const& pos, f32 rot, glm::vec2 const size, glm::vec3 const colour, f32 const ratio);
};
} // namespace le
