#pragma once
#include <cstdint>
#include <core/std_types.hpp>
#include <engine/ibase.hpp>
#include <glm/vec2.hpp>

namespace le {
namespace graphics {
class CommandBuffer;
}
struct Camera;

struct IMaterial : IBase {
	virtual void write(std::size_t) = 0;
	virtual void bind(graphics::CommandBuffer const& cb, std::size_t idx) const = 0;
};

struct IDrawable : IBase {
	virtual void update(std::size_t) = 0;
	virtual void draw(graphics::CommandBuffer const& cb, std::size_t idx) const = 0;
};
} // namespace le
