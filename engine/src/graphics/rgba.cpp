#include <glm/gtc/color_space.hpp>
#include <glm/mat4x4.hpp>
#include <spaced/engine/graphics/rgba.hpp>

namespace spaced::graphics {
auto Rgba::to_srgb(glm::vec4 const& linear) -> glm::vec4 { return glm::convertLinearToSRGB(linear); }
auto Rgba::to_linear(glm::vec4 const& srgb) -> glm::vec4 { return glm::convertSRGBToLinear(srgb); }
} // namespace spaced::graphics
