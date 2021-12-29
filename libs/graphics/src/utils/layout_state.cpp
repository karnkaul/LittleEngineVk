#include <core/utils/expect.hpp>
#include <graphics/command_buffer.hpp>
#include <graphics/memory.hpp>
#include <graphics/utils/layout_state.hpp>

namespace le::graphics {
vk::ImageLayout LayoutState::get(vk::Image img) const {
	auto lock = ktl::klock(m_map);
	[[maybe_unused]] volatile auto sz = lock->size();
	if (auto it = lock->find(img); it != lock->end()) { return it->second; }
	return vk::ImageLayout::eUndefined;
}

void LayoutState::force(vk::Image image, vk::ImageLayout layout) {
	auto lock = ktl::klock(m_map);
	lock->insert_or_assign(image, layout);
}

void LayoutState::transition(vk::CommandBuffer cb, vk::Image img, vk::ImageLayout layout, LayoutStages const& ls, LayerMip const& lm) {
	vk::ImageLayout prev = vk::ImageLayout::eUndefined;
	auto lock = ktl::klock(m_map);
	auto it = lock->find(img);
	if (it == lock->end()) {
		auto [i, _] = lock->emplace(img, prev);
		it = i;
	} else {
		prev = it->second;
	}
	Memory::ImgMeta im;
	im.stages = {ls.src.first, ls.dst.first};
	im.access = {ls.src.second, ls.dst.second};
	im.aspects = ls.aspects;
	im.layouts = {prev, layout};
	im.layerMip = lm;
	Memory::imageBarrier(cb, img, im);
	it->second = layout;
}
} // namespace le::graphics
