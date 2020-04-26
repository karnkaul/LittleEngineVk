#include <algorithm>
#include "engine/gfx/geometry.hpp"

namespace le::gfx
{
void Geometry::reserve(u32 vertCount, u32 indexCount)
{
	vertices.reserve(vertCount);
	indices.reserve(indexCount);
}

u32 Geometry::addVertex(Vertex const& vertex)
{
	u32 ret = (u32)vertices.size();
	vertices.push_back(vertex);
	return ret;
}

void Geometry::addIndices(std::vector<u32> newIndices)
{
	std::copy(newIndices.begin(), newIndices.end(), std::back_inserter(indices));
}
} // namespace le::gfx
