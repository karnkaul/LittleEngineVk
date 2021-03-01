#include <engine/camera.hpp>
#include <engine/render/draw.hpp>
#include <graphics/context/command_buffer.hpp>
#include <graphics/mesh.hpp>
#include <graphics/pipeline.hpp>

namespace le {
Drawer::Drawable::Drawable(std::string_view name, graphics::VRAM& vram, Prop2& prop)
	: mat_m(vram, std::string(name) + "/mat_m", {}), locals(vram, std::string(name) + "/locals", {}), prop(prop) {
}

void Drawer::Drawable::reassign(Prop2& prop) {
	this->prop = prop;
}

void Drawer::Drawable::update(std::size_t idx) {
	auto& input = prop.get().material.get().pPipe->shaderInput();
	if (input.contains(ModelMats::sb.set, ModelMats::sb.bind)) {
		mat_m.write(prop.get().transform.model());
		input.update(mat_m, ModelMats::sb.set, ModelMats::sb.bind, idx);
		mat_m.swap();
	}
	if (input.contains(ObjectAlbedo::sb.set, ObjectAlbedo::sb.bind)) {
		locals.write(prop.get().material.get().albedo);
		input.update(locals, ObjectAlbedo::sb.set, ObjectAlbedo::sb.bind);
	}
	prop.get().material.get().write(idx);
}

void Drawer::Drawable::draw(graphics::CommandBuffer const& cb, std::size_t idx) const {
	graphics::Pipeline const& pi = *prop.get().material.get().pPipe;
	auto& input = pi.shaderInput();
	if (input.contains(ModelMats::sb.set)) {
		cb.bindSet(pi.layout(), input.set(ModelMats::sb.set).index(idx));
	}
	/*if (input.contains(ObjectAlbedo::sb.set)) {
		cb.bindSet(pi.layout(), input.set(ObjectAlbedo::sb.bind).index(idx));
	}*/
	prop.get().material.get().bind(cb, idx);
	prop.get().mesh.get().draw(cb);
}

Drawer::Drawer(graphics::VRAM& vram, decf::registry_t& registry)
	: m_viewMats(vram, "view_mats", {}), m_dirLights(vram, "dir_lights", {}), m_vram(vram), m_registry(registry) {
}

void Drawer::update(DrawView const& view) {
	ENSURE(view.pCamera, "Camera is null");
	Camera const& cam = *view.pCamera;
	glm::vec2 const fb = view.framebuffer;
	ViewMats const v{cam.view(), cam.perspective(fb), cam.ortho(fb), cam.position};
	m_viewMats.write<false>(v);
	if (!view.dirLights.empty()) {
		m_dirLights.write(view.dirLights.front());
	}
	m_lists = toLists();
	for (auto& list : m_lists) {
		auto& si = list.material.get().pPipe->shaderInput();
		si.update(m_viewMats, ViewMats::sb.set, ViewMats::sb.bind, 0);
		if (!view.dirLights.empty() && si.contains(DirLights::sb.set, DirLights::sb.bind)) {
			si.update(m_dirLights, DirLights::sb.set, DirLights::sb.bind, 0);
		}
		list.update();
	}
	m_viewMats.swap();
}

void Drawer::draw(graphics::CommandBuffer const& cb) const {
	std::unordered_set<Ref<graphics::Pipeline>> ps;
	for (auto const& list : m_lists) {
		graphics::Pipeline& pi = *list.material.get().pPipe;
		graphics::ShaderInput& si = pi.shaderInput();
		ps.insert(pi);
		cb.bindPipe(pi);
		cb.bindSet(pi.layout(), si.set(ViewMats::sb.set).front());
		list.draw(cb);
	}
	for (graphics::Pipeline& pipe : ps) {
		pipe.shaderInput().swap();
	}
}

Prop2& Drawer::operator[](decf::entity_t entity) {
	if (auto pD = m_registry.get().find<Drawable>(entity)) {
		return pD->prop;
	}
	ENSURE(false, "Invalid id");
	throw std::runtime_error("Invalid id");
}

Prop2 const& Drawer::operator[](decf::entity_t entity) const {
	if (auto pD = m_registry.get().find<Drawable>(entity)) {
		return pD->prop;
	}
	ENSURE(false, "Invalid id");
	throw std::runtime_error("Invalid id");
}

Prop2& Drawer::spawn(std::string_view name, Ref<graphics::Mesh const> mesh, Ref<MatBlank> material) {
	auto [e, c] = m_registry.get().spawn<Prop2>(std::string(name), mesh, material);
	auto& [prop] = c;
	prop.entity = e;
	return spawnImpl(name, prop);
}

Prop2& Drawer::spawn(std::string_view name, Prop2&& prop) {
	auto [e, c] = m_registry.get().spawn<Prop2>(std::string(name), std::move(prop));
	auto& [p] = c;
	p.entity = e;
	return spawnImpl(name, p);
}

Prop2* Drawer::attach(decf::entity_t entity) {
	if (m_registry.get().contains(entity)) {
		auto pD = m_registry.get().find<Drawable>(entity);
		auto pP = m_registry.get().find<Prop2>(entity);
		if (!pD && !pP) {
			return nullptr;
		} else if (pD && !pP) {
			ENSURE(false, "Prop destroyed / dangling Drawable");
			pD->prop = *pP;
			return pP;
		} else if (pP && !pD) {
			auto name = m_registry.get().name(entity);
			Drawable dr(std::string(name), m_vram, *pP);
			pD = m_registry.get().attach<Drawable>(entity, std::move(dr));
			ENSURE(pD, "Invariant violated");
			pD->prop.get().entity = entity;
		}
		ENSURE(pD, "Invariant violated");
		return &pD->prop.get();
	}
	return nullptr;
}

std::vector<Drawer::List> Drawer::toLists() const {
	auto drawables = m_registry.get().view<Prop2, Drawable>();
	std::vector<Ref<Drawable>> v;
	v.reserve(drawables.size());
	for (auto& [_, d] : drawables) {
		auto& [_2, dr] = d;
		v.push_back(dr);
	}
	return List::to(v);
}

Prop2& Drawer::spawnImpl(std::string_view name, Prop2& out_prop) {
	auto pD = m_registry.get().attach<Drawable>(out_prop.entity, std::string(name), m_vram, out_prop);
	ENSURE(pD, "Invariant violated");
	return pD->prop;
}
} // namespace le
