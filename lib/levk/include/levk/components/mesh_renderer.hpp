#pragma once
#include <levk/mesh.hpp>
#include <levk/scene.hpp>

namespace levk {
class IMeshRenderer : public IRenderComponent {
  public:
	[[nodiscard]] auto get_im_label() const -> CString final { return "MeshRenderer"; }

	[[nodiscard]] virtual auto get_mesh() const -> IMesh const& = 0;
};

class StaticMeshRenderer : public IMeshRenderer {
  public:
	using MeshType = StaticMesh;

	explicit StaticMeshRenderer(NotNull<StaticMesh const*> mesh) : mesh(mesh) {}

	[[nodiscard]] auto get_mesh() const -> IMesh const& final { return *mesh; }

	NotNull<StaticMesh const*> mesh;

  private:
	void tick(Entity& /*entity*/, Seconds /*dt*/) final {}

	void render_to(Entity const& entity, RenderList& render_list) const final {
		render_list.opaque.push_back(get_render_object(entity, mesh->get_primitives(), {}));
	}

	void im_inspect() final;
};

class SkinnedMeshRenderer : public IMeshRenderer {
  public:
	using MeshType = SkinnedMesh;

	explicit SkinnedMeshRenderer(NotNull<SkinnedMesh const*> mesh) : m_mesh(mesh), m_skeleton(mesh->get_skeleton()) {}

	[[nodiscard]] auto get_mesh() const -> IMesh const& final { return *m_mesh; }

	[[nodiscard]] auto get_skinned_mesh() const -> SkinnedMesh const& { return *m_mesh; }
	[[nodiscard]] auto get_skeleton() const -> Skeleton const& { return m_skeleton; }
	[[nodiscard]] auto get_skeleton() -> Skeleton& { return m_skeleton; }

	void set_mesh(NotNull<SkinnedMesh const*> mesh) {
		m_mesh = mesh;
		m_skeleton = mesh->get_skeleton();
	}

  private:
	void tick(Entity& /*entity*/, Seconds dt) final { m_skeleton.tick(dt); }

	void render_to(Entity const& entity, RenderList& render_list) const final {
		auto const render_object = get_render_object(entity, m_mesh->get_primitives(), m_skeleton.get_joint_matrices());
		render_list.opaque.push_back(render_object);
	}

	void im_inspect() final;

	NotNull<SkinnedMesh const*> m_mesh;
	Skeleton m_skeleton{};
};
} // namespace levk
