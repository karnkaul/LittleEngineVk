#pragma once
#include <levk/primitive.hpp>
#include <levk/scene.hpp>

namespace levk {
class PrimitiveRenderer : public IRenderComponent {
  public:
	std::unique_ptr<IPrimitive> primitive{};

  private:
	void tick(Entity& /*entity*/, Seconds /*dt*/) final {}

	void render_to(Entity const& entity, RenderList& render_list) const final {
		if (!primitive) { return; }
		m_primitive = primitive.get();
		if (m_primitive == nullptr) { return; }
		render_list.opaque.push_back(get_render_object(entity, {&*m_primitive, 1}, {}));
	}

	mutable std::optional<NotNull<IPrimitive const*>> m_primitive{};
};
} // namespace levk
