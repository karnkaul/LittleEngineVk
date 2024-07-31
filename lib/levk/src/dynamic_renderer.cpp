#include <levk/dynamic_renderer.hpp>
#include <levk/util.hpp>

namespace levk {
DynamicRenderer::DynamicRenderer(NotNull<IRenderDevice*> device) : m_device(device) {}

auto DynamicRenderer::begin_rendering(RenderTarget const& render_target, vk::CommandBuffer const command_buffer) -> bool {
	if (render_target.is_empty() || !command_buffer) { return false; }
	m_command_buffer = command_buffer;
	m_render_target = render_target;

	pre_transition();

	auto const cc = colour::to_linear(clear_colour);
	auto const colour_attachment = vk::RenderingAttachmentInfo{
		m_render_target.colour.view,
		vk::ImageLayout::eAttachmentOptimal,
		m_render_target.resolve.image ? vk::ResolveModeFlagBits::eAverage : vk::ResolveModeFlagBits::eNone,
		m_render_target.resolve.view,
		vk::ImageLayout::eAttachmentOptimal,
		load_op == LoadOp::eLoad ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::ClearColorValue{std::array{cc.x, cc.y, cc.z, cc.w}},
	};
	auto const depth_attachment = vk::RenderingAttachmentInfo{
		m_render_target.depth.view,
		vk::ImageLayout::eAttachmentOptimal,
		vk::ResolveModeFlagBits::eNone,
		vk::ImageView{},
		vk::ImageLayout::eUndefined,
		vk::AttachmentLoadOp::eClear,
		depth_store_op == StoreOp::eStore ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare,
		vk::ClearDepthStencilValue{1.0f, 0},
	};
	auto const ri = vk::RenderingInfo{
		vk::RenderingFlags{},
		vk::Rect2D{vk::Offset2D{}, m_render_target.extent},
		1,
		0,
		m_render_target.colour.image ? 1u : 0u,
		m_render_target.colour.image ? &colour_attachment : nullptr,
		m_render_target.depth.image ? &depth_attachment : nullptr,
	};
	command_buffer.beginRendering(ri);

	return true;
}

void DynamicRenderer::end_rendering() {
	if (m_render_target.is_empty() || !m_command_buffer) { return; }
	m_command_buffer.endRendering();
	post_transition();
	m_render_target = {};
	m_command_buffer = vk::CommandBuffer{};
}

auto DynamicRenderer::create_barrier(vk::Image image, Transition const transition) const -> vk::ImageMemoryBarrier2 {
	auto ib = vk::ImageMemoryBarrier2{};
	ib.srcQueueFamilyIndex = ib.dstQueueFamilyIndex = m_device->get_queue_family();
	ib.image = image;
	ib.oldLayout = transition.old_layout;
	ib.newLayout = transition.new_layout;
	if (image == m_render_target.depth.image) {
		ib.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};
	} else {
		ib.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
	}
	return ib;
}

void DynamicRenderer::pre_transition() {
	m_barriers.clear();
	if (m_render_target.colour.image) {
		auto const old_layout = load_op == LoadOp::eLoad ? layouts.old_layout : vk::ImageLayout::eUndefined;
		auto barrier = create_barrier(m_render_target.colour.image, {old_layout, vk::ImageLayout::eAttachmentOptimal});
		barrier.srcStageMask = barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eFragmentShader;
		barrier.srcAccessMask = barrier.dstAccessMask =
			vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eShaderSampledRead;
		m_barriers.push_back(barrier);
		if (m_render_target.resolve.image) {
			barrier.image = m_render_target.resolve.image;
			m_barriers.push_back(barrier);
		}
	}
	if (m_render_target.depth.image) {
		auto barrier = create_barrier(m_render_target.depth.image, {vk::ImageLayout::eUndefined, vk::ImageLayout::eAttachmentOptimal});
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eFragmentShader;
		barrier.srcAccessMask =
			vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eShaderSampledRead;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
		barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
		m_barriers.push_back(barrier);
	}
	record_barriers(m_command_buffer, m_barriers);
}

void DynamicRenderer::post_transition() {
	m_barriers.clear();
	if (layouts.new_layout == vk::ImageLayout::eUndefined) { return; }
	if (m_render_target.colour.image) {
		auto barrier = create_barrier(m_render_target.colour.image, {vk::ImageLayout::eAttachmentOptimal, layouts.new_layout});
		barrier.srcStageMask = barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eFragmentShader;
		barrier.srcAccessMask = barrier.dstAccessMask =
			vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eShaderSampledRead;
		m_barriers.push_back(barrier);
		if (m_render_target.resolve.image) {
			barrier.image = m_render_target.resolve.image;
			m_barriers.push_back(barrier);
		}
	}
	if (m_render_target.depth.image) {
		auto barrier = create_barrier(m_render_target.depth.image, {vk::ImageLayout::eAttachmentOptimal, layouts.new_layout});
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
		barrier.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
		barrier.dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead;
		m_barriers.push_back(barrier);
	}
	record_barriers(m_command_buffer, m_barriers);
}
} // namespace levk
