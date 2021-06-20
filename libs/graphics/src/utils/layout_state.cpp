#include <graphics/render/command_buffer.hpp>
#include <graphics/utils/layout_state.hpp>

namespace le::graphics {
vk::ImageLayout LayoutState::get(vk::Image img) const {
	if (auto it = m_map.find(img); it != m_map.end()) { return it->second; }
	return vk::ImageLayout::eUndefined;
}

void LayoutState::transition(Img img, CommandBuffer cb, vk::ImageLayout layout, StageAccess src, StageAccess dst) {
	vk::ImageLayout prev = vk::ImageLayout::eUndefined;
	auto it = m_map.find(img.image);
	if (it != m_map.end()) { prev = it->second; }
	cb.transitionImage(img.image, 1U, img.aspect, {prev, layout}, {src.second, dst.second}, {src.first, dst.first});
	if (it != m_map.end()) {
		it->second = layout;
	} else {
		m_map[img.image] = layout;
	}
}
} // namespace le::graphics
