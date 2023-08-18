#pragma once
#include <le/graphics/render_object.hpp>
#include <le/scene/component.hpp>
#include <le/scene/render_layer.hpp>
#include <vector>

namespace le {
class RenderComponent : public Component {
  public:
	virtual auto render_to(std::vector<graphics::RenderObject>& out) const -> void = 0;

	RenderLayer render_layer{};
};
} // namespace le
