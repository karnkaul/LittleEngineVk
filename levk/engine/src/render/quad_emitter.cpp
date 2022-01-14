#include <core/maths.hpp>
#include <core/utils/expect.hpp>
#include <glm/mat2x2.hpp>
#include <levk/engine/render/quad_emitter.hpp>

namespace le {
namespace {
glm::vec3 rvec3(EmitterInfo::Range<glm::vec3> const& in, bool norm = false) noexcept {
	glm::vec3 ret;
	ret.x = maths::randomRange(in.first.x, in.second.x);
	ret.y = maths::randomRange(in.first.y, in.second.y);
	ret.z = maths::randomRange(in.first.z, in.second.z);
	if (norm) {
		if (glm::length2(ret) == 0.0f) {
			EXPECT(glm::length2(in.first) > 0.0f || glm::length2(in.second) > 0.0f);
			return rvec3(in, true);
		}
		return glm::normalize(ret);
	}
	return ret;
}

f32 rfloat(EmitterInfo::Range<f32> const in) noexcept { return maths::randomRange(in.first, in.second); }
} // namespace

void QuadEmitter::Pos::operator()(Time_s const dt) noexcept {
	elapsed += dt;
	linear.tick(dt);
	radial.tick(dt);
	ratio = elapsed.count() / ttl.count();
}

void QuadEmitter::Geom::operator()(glm::vec3 const& pos, f32 const radz, glm::vec2 const size, glm::vec3 const colour, f32 const ratio) {
	graphics::GeomInfo gi;
	gi.colour = {colour, maths::lerp(alpha.first, alpha.second, ratio)};
	geometry = graphics::makeQuad(size * maths::lerp(scale.first, scale.second, ratio), gi);
	glm::vec2 const cs = {glm::cos(radz), glm::sin(radz)};
	auto const mat = glm::mat2x2{
		{cs.x, cs.y},
		{-cs.y, cs.x},
	};
	for (auto& vert : geometry.vertices) {
		auto const vert_xy = mat * glm::vec2(vert.position);
		vert.position = glm::vec3(vert_xy, vert.position.z) + pos;
	}
}

void QuadEmitter::create(EmitterInfo const& info) {
	m_info = info;
	m_data.gen.clear();
	m_data.pos.clear();
	m_data.gen.reserve(m_info.count);
	m_data.pos.reserve(m_info.count);
	for (std::size_t i = 0; i < m_info.count / 2; ++i) { addQuad(); }
}

void QuadEmitter::tick(Time_s dt, Opt<dts::task_queue> tasks) {
	if (m_info.loop) {
		while (m_data.pos.size() < m_info.count) { addQuad(); }
	}
	auto const colour = m_info.colour.toVec4();
	auto range = [dt, colour, this](std::size_t begin, std::size_t end) {
		for (std::size_t i = begin; i < end; ++i) {
			auto& pos = m_data.pos[i];
			pos(dt);
			m_data.gen[i](pos.linear.position, pos.radial.position, m_info.size, colour, pos.ratio);
		}
	};
	bool b = true;
	static constexpr std::size_t chunk = 1024U;
	if (b && tasks && m_data.pos.size() > chunk) {
		m_data.tids.reserve(m_data.pos.size() / chunk + 1U);
		std::size_t begin = 0;
		std::size_t end = std::min(begin + chunk, m_data.pos.size());
		while (begin < m_data.pos.size()) {
			m_data.tids.push_back(tasks->enqueue([range, begin, end] { range(begin, end); }));
			begin = end;
			end = std::min(begin + chunk, m_data.pos.size());
		}
		tasks->wait_tasks(m_data.tids);
		m_data.tids.clear();
	} else {
		range(0U, m_data.pos.size());
	}
	std::size_t end = m_data.pos.size();
	for (std::size_t i = 0; i < end;) {
		if (m_data.pos[i].ratio >= 1.0f) {
			if (i + 1 < end) {
				std::swap(m_data.pos[i], m_data.pos.back());
				std::swap(m_data.gen[i], m_data.gen.back());
			}
			m_data.pos.pop_back();
			m_data.gen.pop_back();
			EXPECT(end > 0U);
			--end;
		} else {
			++i;
		}
	}
	EXPECT(m_data.gen.size() == m_data.pos.size());
}

graphics::Geometry QuadEmitter::geometry() const {
	graphics::Geometry ret;
	ret.reserve(u32(m_data.gen.size() * 4U), u32(m_data.gen.size()) * 6U);
	for (auto const& geom : m_data.gen) { ret.append(geom.geometry); }
	return ret;
}

void QuadEmitter::addQuad() {
	Pos qp;
	qp.linear.position = rvec3(m_info.init.position);
	qp.linear.velocity = rvec3(m_info.init.linear.direction, true) * rvec3(m_info.init.linear.speed);
	qp.radial.position = 0.0f;
	qp.radial.velocity = glm::radians(rfloat(m_info.init.angular.speed));
	qp.ttl = Time_s(rfloat({m_info.init.ttl.first.count(), m_info.init.ttl.second.count()}));
	m_data.pos.push_back(qp);
	Geom qg;
	auto const scale = rfloat(m_info.init.scale);
	qg.scale = {scale, scale * m_info.end.scale};
	auto const alpha = rfloat(m_info.init.alpha);
	qg.alpha = {alpha, alpha * m_info.end.alpha};
	m_data.gen.push_back(qg);
}
} // namespace le
