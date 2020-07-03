#include <engine/gfx/screen_rect.hpp>

namespace le::gfx
{
ScreenRect::ScreenRect(glm::vec4 const& ltrb) noexcept : left(ltrb.x), top(ltrb.y), right(ltrb.z), bottom(ltrb.w) {}

ScreenRect ScreenRect::sizeTL(glm::vec2 const& size, glm::vec2 const& leftTop)
{
	return ScreenRect({leftTop.x, leftTop.y, leftTop.x + size.x, leftTop.y + size.y});
}

ScreenRect ScreenRect::sizeCentre(glm::vec2 const& size, glm::vec2 const& centre)
{
	auto const leftTop = centre - (glm::vec2(0.5f) * size);
	return ScreenRect({leftTop.x, leftTop.y, leftTop.x + size.x, leftTop.y + size.y});
}

glm::vec2 ScreenRect::midPoint() const
{
	return {(right + left) * 0.5f, (bottom + top) * 0.5f};
}

glm::vec2 ScreenRect::size() const
{
	return glm::vec2(right - left, bottom - top);
}

f32 ScreenRect::aspect() const
{
	glm::vec2 const s = size();
	return s.x / s.y;
}

ScreenRect ScreenRect::adjust(const ScreenRect& by) const
{
	glm::vec2 const s = size() * by.size();
	return ScreenRect({left + by.left, top + by.top, left + by.left + s.x, top + by.top + s.y});
}
} // namespace le::gfx
