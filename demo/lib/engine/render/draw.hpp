#pragma once
#include <vector>
#include <core/ec_registry.hpp>
#include <core/hash.hpp>
#include <core/transform.hpp>
#include <core/utils/std_hash.hpp>
#include <engine/render/material.hpp>
#include <glm/mat4x4.hpp>
#include <graphics/shader_buffer.hpp>

namespace le {
namespace graphics {
class Mesh;
}

struct ViewMats {
	glm::mat4 mat_p;
	glm::mat4 mat_v;
	glm::mat4 mat_ui;
};

struct Prop2 {
	ec::Entity entity;
	Transform transform;
	Ref<graphics::Mesh const> mesh;
	Ref<MatBlank> material;

	Prop2(graphics::Mesh const& mesh, MatBlank& material);
};

class Drawer {
  public:
	Drawer(graphics::VRAM& vram, ec::Registry& registry);

	void update(Camera const& cam, glm::ivec2 fb);
	void draw(graphics::CommandBuffer const& cb) const;

	Prop2& spawn(std::string_view name, Ref<graphics::Mesh const> mesh, Ref<MatBlank> material);
	Prop2& spawn(std::string_view name, Prop2&& prop);
	Prop2* attach(ec::Entity entity);

	Prop2& operator[](ec::Entity entity);
	Prop2 const& operator[](ec::Entity entiy) const;

  private:
	struct Drawable : IDrawable {
		graphics::ShaderBuffer local;
		Ref<Prop2> prop;

		Drawable(std::string_view name, graphics::VRAM& vram, Prop2& prop);

		void reassign(Prop2& prop);

		void update(std::size_t idx) override;
		void draw(graphics::CommandBuffer const& cb, std::size_t idx) const override;
	};

	struct List;

	std::vector<Drawer::List> toLists() const;
	Prop2& spawnImpl(std::string_view name, Prop2& out_prop);

	graphics::ShaderBuffer m_view;
	std::vector<List> m_lists;
	Ref<graphics::VRAM> m_vram;
	Ref<ec::Registry> m_registry;
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
		map[d.prop.get().material].push_back(d);
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
} // namespace le
