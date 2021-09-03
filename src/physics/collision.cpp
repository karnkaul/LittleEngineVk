#include <algorithm>
#include <core/log.hpp>
#include <core/services.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/physics/collision.hpp>
#include <glm/vec2.hpp>
#include <graphics/mesh.hpp>

namespace le {
namespace {
constexpr bool intersecting(glm::vec2 l, glm::vec2 r) noexcept { return (l.x >= r.x && l.x <= r.y) || (l.y >= r.x && l.y <= r.y); }

constexpr glm::vec2 lohi(f32 pos, f32 scale, f32 offset) noexcept { return {pos + scale * -0.5f + offset, pos + scale * 0.5f + offset}; }

bool colliding(Collision::Rect const& lr, Collision::Rect const& rr, glm::vec3 const& lp, glm::vec3 const& rp) noexcept {
	glm::vec2 const lx = lohi(lp.x, lr.scale.x, lr.offset.x);
	glm::vec2 const rx = lohi(rp.x, rr.scale.x, rr.offset.x);
	glm::vec2 const ly = lohi(lp.y, lr.scale.y, lr.offset.y);
	glm::vec2 const ry = lohi(rp.y, rr.scale.y, rr.offset.y);
	glm::vec2 const lz = lohi(lp.z, lr.scale.z, lr.offset.z);
	glm::vec2 const rz = lohi(rp.z, rr.scale.z, rr.offset.z);
	return intersecting(lx, rx) && intersecting(ly, ry) && intersecting(lz, rz);
}
} // namespace

void Collision::Collider::channels(u32 set, u32 unset) const noexcept {
	*m_cflags &= ~unset;
	*m_cflags |= set;
}

void Collision::Data::push_back(ID id, Rect rect, glm::vec3 pos, CFlags flags) {
	ids.push_back(id);
	rects.push_back(rect);
	positions.push_back(pos);
	cflags.push_back(flags);
	onCollides.emplace_back();
	Prop prop;
	if (auto store = Services::find<AssetStore>()) {
		if (auto mesh = store->find<graphics::Mesh>("wireframes/cube")) {
			prop.mesh = &*mesh;
			prop.material.Tf = colours::green;
		}
	}
	props.push_back(prop);
}

template <typename... T>
void increment(T... it) {
	(++it, ...);
}

template <typename... T>
void clearAll(T&... out) {
	(out.clear(), ...);
}

bool Collision::Data::erase(ID id) noexcept {
	auto iti = ids.begin();
	auto itr = rects.begin();
	auto itp = positions.begin();
	auto itch = cflags.begin();
	auto itcl = onCollides.begin();
	auto itpr = props.begin();
	auto erase = [](auto& vec, auto it) {
		if (vec.size() > 1) { std::swap(*(vec.end() - 1), *it); }
		vec.pop_back();
	};
	for (; iti != ids.end(); ++iti) {
		if (*iti == id) {
			erase(ids, iti);
			erase(rects, itr);
			erase(positions, itp);
			erase(cflags, itch);
			erase(onCollides, itcl);
			erase(props, itpr);
			return true;
		}
		increment(itr, itp, itch, itcl, itpr);
	}
	return false;
}

void Collision::Data::clear() noexcept { clearAll(ids, rects, positions, cflags, onCollides, props); }

Collision::Collider Collision::add(Rect rect, glm::vec3 position, CFlags cflags) {
	m_data.push_back(++m_nextID, rect, position, cflags);
	return cube(m_data.ids.size() - 1);
}

std::optional<Collision::Collider> Collision::find(ID id) noexcept {
	for (auto const [id_, idx] : utils::enumerate(m_data.ids)) {
		if (id_ == id) { return cube(idx); }
	}
	return std::nullopt;
}

void Collision::update() {
	for (std::size_t i = 0; i + 1 < m_data.ids.size(); ++i) {
		for (std::size_t j = i + 1; j < m_data.ids.size(); ++j) { update(i, j); }
	}
}

std::vector<Drawable> Collision::drawables() const {
	std::vector<Drawable> ret;
	if (m_issueDrawables) {
		for (std::size_t i = 0; i < m_data.ids.size(); ++i) {
			if (m_data.props[i].mesh) {
				Drawable drawable;
				drawable.props = m_data.props[i];
				static constexpr auto idty = glm::mat4(1.0f);
				drawable.model = glm::translate(idty, m_data.positions[i] + m_data.rects[i].offset) * glm::scale(idty, m_data.rects[i].scale);
				ret.push_back(drawable);
			}
		}
	}
	return ret;
}

Collision::Collider Collision::cube(std::size_t i) noexcept {
	ensure(i < m_data.ids.size(), "Invalid index");
	return {m_data.ids[i], &m_data.rects[i], &m_data.positions[i], &m_data.cflags[i], &m_data.onCollides[i]};
}

void Collision::update(std::size_t i, std::size_t j) noexcept {
	bool const ch = (m_data.cflags[i] & m_data.cflags[j]) != 0;
	if (ch && colliding(m_data.rects[i], m_data.rects[j], m_data.positions[i], m_data.positions[j])) {
		m_data.onCollides[i](cube(j));
		m_data.onCollides[j](cube(i));
		m_data.props[i].material.Tf = m_data.props[j].material.Tf = colours::red;
	} else {
		m_data.props[i].material.Tf = m_data.props[j].material.Tf = colours::green;
	}
}
} // namespace le
