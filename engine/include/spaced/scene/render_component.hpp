#pragma once
#include <spaced/graphics/render_object.hpp>
#include <spaced/scene/component.hpp>
#include <vector>

namespace spaced {
enum struct Layer : std::uint32_t {
	eDefault = 0,
};

class RenderComponent : public Component {
  public:
	virtual auto render_to(std::vector<graphics::RenderObject>& out) const -> void = 0;

	Layer layer{};
};
} // namespace spaced
