#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <core/std_types.hpp>

namespace le::gfx
{
struct ScreenRect final
{
	f32 left = 0.0f;
	f32 top = 0.0f;
	f32 right = 1.0f;
	f32 bottom = 1.0f;

	constexpr ScreenRect() noexcept = default;
	ScreenRect(glm::vec4 const& ltrb) noexcept;

	static ScreenRect sizeTL(glm::vec2 const& size, glm::vec2 const& leftTop = glm::vec2(0.0f));
	static ScreenRect sizeCentre(glm::vec2 const& size, glm::vec2 const& centre = glm::vec2(0.5f));

	glm::vec2 size() const;
	f32 aspect() const;

	ScreenRect adjust(ScreenRect const& by) const;
};
} // namespace le::gfx
