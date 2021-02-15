#pragma once
#include <unordered_map>
#include <vector>
#include <core/hash.hpp>
#include <core/transform.hpp>
#include <core/utils/std_hash.hpp>
#include <engine/render/material.hpp>
#include <glm/mat4x4.hpp>
#include <graphics/shader_buffer.hpp>

namespace le {
namespace graphics {
class Mesh;
} // namespace graphics

struct ViewMats {
	glm::mat4 mat_p;
	glm::mat4 mat_v;
	glm::mat4 mat_ui;
};

struct Prop2 {
	Transform transform;
	Ref<graphics::Mesh const> mesh;
	Ref<MatBlank> material;

	Prop2(graphics::Mesh const& mesh, MatBlank& material);
};

class Drawer {
  public:
	Drawer(graphics::VRAM& vram);

	void update(Camera const& cam, glm::ivec2 fb);
	void draw(graphics::CommandBuffer const& cb) const;

	Prop2& add(std::string_view id, Ref<graphics::Mesh const> mesh, Ref<MatBlank> material);
	Prop2& add(std::string_view id, Prop2&& prop);
	bool remove(Hash id) noexcept;
	bool contains(Hash id) const noexcept;
	void clear() noexcept;

	Prop2& operator[](Hash id);
	Prop2 const& operator[](Hash id) const;

  private:
	struct Drawable : IDrawable {
		graphics::ShaderBuffer local;
		Prop2 prop;

		Drawable(std::string_view name, graphics::VRAM& vram, graphics::Mesh const& mesh, MatBlank& mat);
		Drawable(std::string_view name, graphics::VRAM& vram, Prop2&& prop);

		void reassign(graphics::Mesh const& mesh, MatBlank& mat);
		void reassign(Prop2&& prop);

		void update(std::size_t idx) override;
		void draw(graphics::CommandBuffer const& cb, std::size_t idx) const override;
	};

	struct List;

	template <typename M>
	static std::vector<Drawer::List> toLists(M&& props);
	template <typename... T>
	Prop2& addImpl(std::string_view id, T&&... t);
	Prop2& insert(std::string_view id, Drawable&& d);
	Drawable* find(Hash id) noexcept;

	graphics::ShaderBuffer m_view;
	std::unordered_map<Hash, Drawable> m_drawables;
	std::vector<List> m_lists;
	Ref<graphics::VRAM> m_vram;
};

struct Drawer::List {
	Ref<MatBlank> material;
	std::vector<Ref<Drawable>> drawables;

	template <typename C>
	static std::vector<List> to(C&& props);

	void update();
	void draw(graphics::CommandBuffer const& cb) const;
};

// impl

inline Prop2::Prop2(graphics::Mesh const& mesh, MatBlank& material) : mesh(mesh), material(material) {
}

template <typename C>
std::vector<Drawer::List> Drawer::List::to(C&& props) {
	std::unordered_map<Ref<MatBlank>, std::vector<Ref<Drawable>>> map;
	for (Drawable& d : props) {
		map[d.prop.material].push_back(d);
	}
	std::vector<List> ret;
	for (auto const& [mat, props] : map) {
		List list{mat, {}};
		list.drawables = std::move(props);
		ret.push_back(std::move(list));
	}
	std::sort(ret.begin(), ret.end(), [](List const& a, List const& b) { return a.material.get().layer.order < b.material.get().layer.order; });
	return ret;
}
inline void Drawer::List::update() {
	for (std::size_t idx = 0; idx < drawables.size(); ++idx) {
		drawables[idx].get().update(idx);
	}
}
inline void Drawer::List::draw(graphics::CommandBuffer const& cb) const {
	for (std::size_t idx = 0; idx < drawables.size(); ++idx) {
		drawables[idx].get().draw(cb, idx);
	}
}

template <typename M>
std::vector<Drawer::List> Drawer::toLists(M&& m) {
	std::vector<Ref<Drawable>> v;
	v.reserve(m.size());
	for (auto& [_, d] : m) {
		v.push_back(d);
	}
	return List::to(v);
}

template <typename... T>
Prop2& Drawer::addImpl(std::string_view id, T&&... t) {
	if (auto d = find(id)) {
		ENSURE(false, "Overwriting existing Prop!");
		d->reassign(std::forward<T>(t)...);
		return d->prop;
	}
	return insert(id, Drawable(id, m_vram, std::forward<T>(t)...));
}
} // namespace le
