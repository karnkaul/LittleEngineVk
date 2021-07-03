#include <graphics/context/device.hpp>
#include <graphics/render/renderer_fwd_swp.hpp>

namespace le::graphics {
RendererFwdSwp::RendererFwdSwp(not_null<Swapchain*> swapchain, Buffering buffering) : ARenderer(swapchain, buffering) {
	m_storage = make({}, Transition::eCommandBuffer);
}

std::optional<RendererFwdSwp::Draw> RendererFwdSwp::beginFrame() {
	if (auto acq = acquire()) {
		RenderTarget const target{acq->image, depthImage(acq->image.extent)};
		return Draw{target, m_storage.buf.get().cb};
	}
	return std::nullopt;
}

void RendererFwdSwp::endDraw(RenderTarget const& target) {
	auto& buf = m_storage.buf.get();
	buf.cb.endRenderPass();
	m_device->m_layouts.transition<lt::TransferPresent>(buf.cb, target.colour.image);
	m_device->m_layouts.drawn(target.depth.image);
}
} // namespace le::graphics
