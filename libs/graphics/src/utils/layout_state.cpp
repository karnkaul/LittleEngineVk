#include <graphics/command_buffer.hpp>
#include <graphics/utils/layout_state.hpp>

namespace le::graphics {
vk::ImageLayout LayoutState::get(vk::Image img) const {
	auto lock = ktl::tlock(m_map);
	if (auto it = lock->find(img); it != lock->end()) { return it->second; }
	return vk::ImageLayout::eUndefined;
}

void LayoutState::transition(CommandBuffer cb, vk::Image img, vk::ImageLayout layout, LayoutStages const& ls, u32 layers, u32 mips) {
	vk::ImageLayout prev = vk::ImageLayout::eUndefined;
	auto lock = ktl::tlock(m_map);
	auto it = lock->find(img);
	if (it != lock->end()) { prev = it->second; }
	cb.transitionImage(img, layers, mips, 0, ls.aspects, {prev, layout}, {ls.src.second, ls.dst.second}, {ls.src.first, ls.dst.first});
	if (it != lock->end()) {
		it->second = layout;
	} else {
		lock->emplace(img, layout);
	}
}
} // namespace le::graphics
