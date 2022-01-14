#include <graphics/utils/utils.hpp>
#include <levk/engine/render/pipeline.hpp>
#include <algorithm>

namespace le {
bool RenderPipeline::operator==(RenderPipeline const& rhs) const {
	if (layer != rhs.layer || shaderURIs.size() != rhs.shaderURIs.size()) { return false; }
	auto f = [&rhs](std::string const& uri) { return std::find(rhs.shaderURIs.begin(), rhs.shaderURIs.end(), uri) != rhs.shaderURIs.end(); };
	return std::all_of(shaderURIs.begin(), shaderURIs.end(), f);
}

std::size_t RenderPipeline::Hasher::operator()(RenderPipeline const& rp) const {
	graphics::utils::HashGen ret;
	ret << rp.layer.order << rp.layer.mode << rp.layer.topology << rp.layer.flags.bits() << rp.layer.lineWidth;
	for (auto const& uri : rp.shaderURIs) { ret << std::hash<std::string>{}(uri); }
	return ret;
}
} // namespace le
