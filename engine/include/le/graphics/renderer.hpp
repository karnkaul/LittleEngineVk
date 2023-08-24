#pragma once
#include <le/core/inclusive_range.hpp>
#include <le/core/mono_instance.hpp>
#include <le/graphics/cache/descriptor_cache.hpp>
#include <le/graphics/cache/pipeline_cache.hpp>
#include <le/graphics/cache/sampler_cache.hpp>
#include <le/graphics/cache/scratch_buffer_cache.hpp>
#include <le/graphics/cache/vertex_buffer_cache.hpp>
#include <le/graphics/dear_imgui.hpp>
#include <le/graphics/defer.hpp>
#include <le/graphics/fallback.hpp>
#include <le/graphics/render_frame.hpp>
#include <le/graphics/swapchain.hpp>
#include <optional>
#include <span>

namespace le::graphics {
struct RenderTarget {
	ImageView colour{};
	ImageView depth{};
};

class Renderer : public MonoInstance<Renderer> {
  public:
	static constexpr auto to_vsync_string(vk::PresentModeKHR mode) -> std::string_view;

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
	[[nodiscard]] auto get_line_width_limit() const -> InclusiveRange<float> { return m_line_width_limit; }

	[[nodiscard]] auto wait_for_frame(glm::uvec2 framebuffer_extent) -> std::optional<std::uint32_t>;
	auto render(RenderFrame const& render_frame, std::uint32_t image_index) -> std::uint32_t;
	auto submit_frame(std::uint32_t image_index) -> bool;

	[[nodiscard]] auto get_supported_present_modes() const -> std::span<vk::PresentModeKHR const> { return m_swapchain.present_modes; }
	[[nodiscard]] auto get_present_mode() const -> vk::PresentModeKHR { return m_swapchain.create_info.presentMode; }

	auto recreate_swapchain(std::optional<glm::uvec2> extent, std::optional<vk::PresentModeKHR> mode) -> bool;
	auto recreate_swapchain(glm::uvec2 const extent) -> bool { return recreate_swapchain(extent, {}); }
	auto recreate_swapchain(vk::PresentModeKHR const mode) -> bool { return recreate_swapchain({}, mode); }

	auto set_imgui(std::unique_ptr<DearImGui> imgui) -> void { m_imgui = std::move(imgui); }

	auto bind_pipeline(vk::Pipeline pipeline) -> bool;
	auto set_viewport(vk::Viewport viewport = {}) -> bool;
	auto set_scissor(vk::Rect2D scissor) -> bool;

	std::optional<glm::vec2> custom_world_frustum{};
	glm::vec3 shadow_frustum{100.0f};
	vk::Extent2D shadow_map_extent{2048, 2048};
	vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};

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
		Buffered<std::unique_ptr<Image>> shadow_maps{};
		Buffered<Sync> syncs{};
		FrameIndex frame_index{};

		glm::uvec2 framebuffer_extent{};
		glm::uvec2 backbuffer_extent{};
		glm::mat4 primary_light_mat{1.0f};
		vk::Pipeline last_bound{};

		static auto make(vk::Device device, std::uint32_t queue_family, vk::Format depth_format) -> Frame;
	};

	struct Std430Instance {
		glm::mat4 transform;
		glm::vec4 tint;
	};

	[[nodiscard]] auto acquire_next_image(glm::uvec2 framebuffer_extent) -> std::optional<std::uint32_t>;
	auto bake_objects(std::span<RenderObject const> objects, std::vector<RenderObject::Baked>& out) -> void;
	auto bake_objects(RenderFrame const& render_frame) -> void;

	std::unique_ptr<DearImGui> m_imgui{};
	PipelineCache m_pipeline_cache{};
	SamplerCache m_sampler_cache{};
	DescriptorCache m_descriptor_cache{};
	ScratchBufferCache m_scratch_buffer_cache{};
	VertexBufferCache m_vertex_buffer_cache{};
	Swapchain m_swapchain{};
	Frame m_frame{};
	DeferQueue m_defer{};

	Fallback m_fallback{};

	std::vector<Std430Instance> m_instances{};
	std::vector<RenderObject::Baked> m_scene_objects{};
	std::vector<RenderObject::Baked> m_ui_objects{};
	InclusiveRange<float> m_line_width_limit{};

	bool m_rendering{};
};

constexpr auto Renderer::to_vsync_string(vk::PresentModeKHR const mode) -> std::string_view {
	switch (mode) {
	case vk::PresentModeKHR::eFifo: return "classic";
	case vk::PresentModeKHR::eFifoRelaxed: return "adaptive";
	case vk::PresentModeKHR::eMailbox: return "mailbox";
	case vk::PresentModeKHR::eImmediate: return "immediate";
	default: return "unsupported";
	}
}

constexpr auto Renderer::to_rect2d(vk::Viewport const& rect) -> vk::Rect2D {
	glm::ivec2 const offset = glm::vec2{rect.x, rect.y};
	glm::uvec2 const extent = glm::vec2{rect.width, rect.height};
	return vk::Rect2D{{offset.x, offset.y}, {extent.x, extent.y}};
}
} // namespace le::graphics
