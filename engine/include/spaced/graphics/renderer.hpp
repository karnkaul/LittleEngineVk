#pragma once
#include <spaced/core/mono_instance.hpp>
#include <spaced/graphics/cache/descriptor_cache.hpp>
#include <spaced/graphics/cache/pipeline_cache.hpp>
#include <spaced/graphics/cache/sampler_cache.hpp>
#include <spaced/graphics/cache/scratch_buffer_cache.hpp>
#include <spaced/graphics/dear_imgui.hpp>
#include <spaced/graphics/defer.hpp>
#include <spaced/graphics/fallback.hpp>
#include <spaced/graphics/subpass.hpp>
#include <spaced/graphics/swapchain.hpp>
#include <optional>

namespace spaced::graphics {
class Renderer : public MonoInstance<Renderer> {
  public:
	Renderer(Renderer const&) = delete;
	Renderer(Renderer&&) = delete;
	auto operator=(Renderer const&) -> Renderer& = delete;
	auto operator=(Renderer&&) -> Renderer& = delete;

	explicit Renderer(glm::uvec2 framebuffer_extent);
	~Renderer();

	[[nodiscard]] static constexpr auto to_viewport(glm::vec2 extent) -> vk::Viewport { return vk::Viewport{0.0f, 0.0f, extent.x, extent.y}; }
	[[nodiscard]] static constexpr auto to_rect2d(vk::Viewport const& rect) -> vk::Rect2D;

	[[nodiscard]] auto get_frame_index() const -> FrameIndex { return m_frame.frame_index; }
	[[nodiscard]] auto get_colour_format() const -> vk::Format;
	[[nodiscard]] auto get_depth_format() const -> vk::Format;

	[[nodiscard]] auto get_pipeline_cache() const -> PipelineCache const& { return m_pipeline_cache; }
	[[nodiscard]] auto get_pipeline_cache() -> PipelineCache& { return m_pipeline_cache; }
	[[nodiscard]] auto get_pipeline_format() const -> PipelineFormat { return {get_colour_format(), get_depth_format()}; }
	[[nodiscard]] auto get_shader_layout() const -> ShaderLayout const& { return m_pipeline_cache.shader_layout(); }
	[[nodiscard]] auto get_dear_imgui() const -> DearImGui& { return *m_imgui; }

	[[nodiscard]] auto wait_for_frame(glm::uvec2 framebuffer_extent) -> std::optional<std::uint32_t>;
	auto render(std::span<NotNull<Subpass*> const> passes, std::uint32_t image_index) -> void;
	auto submit_frame(std::uint32_t image_index) -> bool;

	auto recreate_swapchain(std::optional<glm::uvec2> extent, std::optional<vk::PresentModeKHR> mode) -> bool;
	auto recreate_swapchain(glm::uvec2 const extent) -> bool { return recreate_swapchain(extent, {}); }
	auto recreate_swapchain(vk::PresentModeKHR const mode) -> bool { return recreate_swapchain({}, mode); }

	auto set_imgui(std::unique_ptr<DearImGui> imgui) -> void { m_imgui = std::move(imgui); }

	auto bind_pipeline(vk::Pipeline pipeline) -> bool;
	auto set_viewport(vk::Viewport viewport = {}) -> bool;
	auto set_scissor(vk::Rect2D scissor) -> bool;

  private:
	struct Frame {
		struct Sync {
			vk::UniqueSemaphore draw{};
			vk::UniqueSemaphore present{};
			vk::UniqueFence drawn{};
			vk::UniqueCommandPool command_pool{};
			vk::CommandBuffer command_buffer{};
		};

		Buffered<std::unique_ptr<Image>> depth_images{};
		Buffered<Sync> syncs{};
		FrameIndex frame_index{};

		glm::uvec2 framebuffer_extent{};
		vk::Pipeline last_bound{};

		static auto make(vk::Device device, std::uint32_t queue_family, vk::Format depth_format) -> Frame;
	};

	[[nodiscard]] auto acquire_next_image(glm::uvec2 framebuffer_extent) -> std::optional<std::uint32_t>;

	std::unique_ptr<DearImGui> m_imgui{};
	PipelineCache m_pipeline_cache;
	SamplerCache m_sampler_cache{};
	DescriptorCache m_descriptor_allocator{};
	ScratchBufferCache m_scratch_buffer_allocator{};
	Swapchain m_swapchain{};
	Frame m_frame{};
	DeferQueue m_defer{};

	Fallback m_fallback{};

	Ptr<Subpass> m_current_pass{};
	bool m_rendering{};
};

constexpr auto Renderer::to_rect2d(vk::Viewport const& rect) -> vk::Rect2D {
	glm::ivec2 const offset = glm::vec2{rect.x, rect.y};
	glm::uvec2 const extent = glm::vec2{rect.width, rect.height};
	return vk::Rect2D{{offset.x, offset.y}, {extent.x, extent.y}};
}
} // namespace spaced::graphics
