#include <core/rect2.hpp>

namespace le
{
Rect2::Rect2(glm::vec2 const& size, glm::vec2 const& centre)
{
	glm::vec2 half = size * 0.5f;
	bl = centre - half;
	tr = centre + half;
}

glm::vec2 Rect2::size() const
{
	return {tr.x - bl.x, tr.y - bl.y};
}

glm::vec2 Rect2::centre() const
{
	return {(tr.x + bl.x) * 0.5f, (tr.y + bl.y) * 0.5f};
}

glm::vec2 Rect2::tl() const
{
	return {bl.x, tr.y};
}

glm::vec2 Rect2::br() const
{
	return {tr.x, bl.y};
}

f32 Rect2::aspect() const
{
	auto const s = size();
	return s.x / s.y;
}
} // namespace le
