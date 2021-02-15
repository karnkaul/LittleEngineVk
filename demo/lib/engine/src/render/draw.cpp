#include <engine/camera.hpp>
#include <engine/render/draw.hpp>
#include <graphics/context/command_buffer.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/mesh.hpp>

namespace le {
Drawer::Drawable::Drawable(std::string_view name, graphics::VRAM& vram, graphics::Mesh const& mesh, MatBlank& mat)
	: local(graphics::ShaderBuffer(vram, name, {})), prop(mesh, mat) {
}

Drawer::Drawable::Drawable(std::string_view name, graphics::VRAM& vram, Prop2&& prop) : local(graphics::ShaderBuffer(vram, name, {})), prop(std::move(prop)) {
}

void Drawer::Drawable::reassign(graphics::Mesh const& mesh, MatBlank& mat) {
	prop = Prop2(mesh, mat);
}

void Drawer::Drawable::reassign(Prop2&& prop) {
	this->prop = std::move(prop);
}

void Drawer::Drawable::update(std::size_t idx) {
	auto& input = prop.material.get().pPipe->shaderInput();
	if (input.contains(1)) {
		local.write(prop.transform.model());
		input.update(local, 1, 0, idx);
		local.swap();
	}
	prop.material.get().write(idx);
}

void Drawer::Drawable::draw(graphics::CommandBuffer const& cb, std::size_t idx) const {
	graphics::Pipeline const& pi = *prop.material.get().pPipe;
	auto& input = pi.shaderInput();
	if (input.contains(1)) {
		cb.bindSet(pi.layout(), input.set(1).index(idx));
	}
	prop.material.get().bind(cb, idx);
	prop.mesh.get().draw(cb);
}
Drawer::Drawer(graphics::VRAM& vram) : m_view(vram, "vp", {}), m_vram(vram) {
}

void Drawer::update(Camera const& cam, glm::ivec2 fb) {
	ViewMats const v{cam.perspective(fb), cam.view(), cam.ortho(fb)};
	m_view.write<false>(v);
	m_lists = toLists(m_drawables);
	for (auto& list : m_lists) {
		list.material.get().pPipe->shaderInput().update(m_view, 0, 0, 0);
		list.update();
	}
	m_view.swap();
}

void Drawer::draw(graphics::CommandBuffer const& cb) const {
	std::unordered_set<Ref<graphics::Pipeline>> ps;
	for (auto const& list : m_lists) {
		graphics::Pipeline& pi = *list.material.get().pPipe;
		ps.insert(pi);
		cb.bindPipe(pi);
		cb.bindSet(pi.layout(), pi.shaderInput().set(0).front());
		list.draw(cb);
	}
	for (graphics::Pipeline& pipe : ps) {
		pipe.shaderInput().swap();
	}
}

bool Drawer::remove(Hash id) noexcept {
	if (auto it = m_drawables.find(id); it != m_drawables.end()) {
		m_drawables.erase(it);
		return true;
	}
	return false;
}

bool Drawer::contains(Hash id) const noexcept {
	return m_drawables.find(id) != m_drawables.end();
}

void Drawer::clear() noexcept {
	m_drawables.clear();
}

Prop2& Drawer::operator[](Hash id) {
	if (auto it = m_drawables.find(id); it != m_drawables.end()) {
		return it->second.prop;
	}
	ENSURE(false, "Invalid id");
	throw std::runtime_error("Invalid id");
}

Prop2 const& Drawer::operator[](Hash id) const {
	if (auto it = m_drawables.find(id); it != m_drawables.end()) {
		return it->second.prop;
	}
	ENSURE(false, "Invalid id");
	throw std::runtime_error("Invalid id");
}

Prop2& Drawer::add(std::string_view id, Ref<graphics::Mesh const> mesh, Ref<MatBlank> material) {
	return addImpl(id, mesh, material);
}

Prop2& Drawer::add(std::string_view id, Prop2&& prop) {
	return addImpl(id, std::move(prop));
}

Prop2& Drawer::insert(std::string_view id, Drawable&& d) {
	auto [it, _] = m_drawables.emplace(id, std::move(d));
	return it->second.prop;
}

Drawer::Drawable* Drawer::find(Hash id) noexcept {
	if (auto it = m_drawables.find(id); it != m_drawables.end()) {
		return &it->second;
	}
	return nullptr;
}
} // namespace le
