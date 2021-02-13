#pragma once
#include <cstdint>
#include <engine/ibase.hpp>
#include <glm/vec2.hpp>

namespace le {
namespace graphics {
class CommandBuffer;
}
struct Camera;

struct IMaterial : IBase {
	virtual void write(std::size_t) = 0;
	virtual void bind(graphics::CommandBuffer const&, std::size_t) const = 0;
};

struct IDrawable : IBase {
	virtual void update(std::size_t) = 0;
	virtual void draw(graphics::CommandBuffer const&, std::size_t) const = 0;
};

struct IScene : IBase {
	virtual void update(Camera const&, glm::ivec2) = 0;
	virtual void draw(graphics::CommandBuffer const& cb) const = 0;
};

struct IRenderer : IBase {
	virtual void render() = 0;
};
} // namespace le
