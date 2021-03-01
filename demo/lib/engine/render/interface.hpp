#pragma once
#include <cstdint>
#include <core/std_types.hpp>
#include <engine/ibase.hpp>
#include <glm/vec3.hpp>

namespace le {
namespace graphics {
class CommandBuffer;
}
struct Camera;

struct SetBind {
	u32 set = 0;
	u32 bind = 0;
};

struct Albedo {
	alignas(16) glm::vec3 ambient;
	alignas(16) glm::vec3 diffuse;
	alignas(16) glm::vec3 specular;
};

namespace albedos {
constexpr Albedo empty = {};
constexpr Albedo full = {glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f)};
constexpr Albedo lit = {glm::vec3(0.5f), glm::vec3(1.0f), glm::vec3(0.4f)};
} // namespace albedos

struct IMaterial : IBase {
	virtual void write(std::size_t) = 0;
	virtual void bind(graphics::CommandBuffer const& cb, std::size_t idx) const = 0;
};

struct IDrawable : IBase {
	virtual void update(std::size_t) = 0;
	virtual void draw(graphics::CommandBuffer const& cb, std::size_t idx) const = 0;
};
} // namespace le
