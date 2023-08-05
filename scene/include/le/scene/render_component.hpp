#pragma once
#include <le/graphics/render_object.hpp>
#include <le/scene/component.hpp>
#include <vector>

namespace le {
enum struct Layer : std::int32_t {
	eDefault = 0,
};

class RenderComponent : public Component {
  public:
	virtual auto render_to(std::vector<graphics::RenderObject>& out) const -> void = 0;

	Layer layer{};
};
} // namespace le
