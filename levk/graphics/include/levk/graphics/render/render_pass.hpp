#pragma once
#include <levk/core/not_null.hpp>
#include <levk/graphics/command_buffer.hpp>
#include <levk/graphics/render/framebuffer.hpp>
#include <levk/graphics/rgba.hpp>
#include <levk/graphics/screen_rect.hpp>
#include <levk/graphics/utils/defer.hpp>

namespace le::graphics {
class PipelineFactory;

static constexpr u8 max_secondary_cmd_v = 8;

struct RenderBegin {
	RGBA clear;
	DepthStencil depth = {1.0f, 0};
	ScreenView view{};
};

struct RenderInfo {
	CommandBuffer primary;
	ktl::fixed_vector<CommandBuffer, max_secondary_cmd_v> secondary;
	Framebuffer framebuffer;
	RenderBegin begin;
	vk::RenderPass renderPass;
};

class RenderPass {
  public:
	RenderPass(not_null<Device*> device, not_null<PipelineFactory*> factory, RenderInfo info);

	Framebuffer const& framebuffer() const noexcept { return m_info.framebuffer; }
	ScreenView const& view() const noexcept { return m_info.begin.view; }
	vk::RenderPass renderPass() const noexcept { return m_info.renderPass; }
	RenderInfo const& info() const noexcept { return m_info; }
	PipelineFactory& pipelineFactory() const noexcept { return *m_factory; }
	Span<CommandBuffer const> commandBuffers() const noexcept { return m_info.secondary.empty() ? Span(m_info.primary) : m_info.secondary; }

	vk::Viewport viewport() const;
	vk::Rect2D scissor() const;

	void end();

  private:
	void beginPass();

	RenderInfo m_info;
	Defer<vk::Framebuffer> m_framebuffer;
	not_null<Device*> m_device;
	not_null<PipelineFactory*> m_factory;
};
} // namespace le::graphics
