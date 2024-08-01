#pragma once
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <levk/buffering.hpp>
#include <levk/core/c_string.hpp>
#include <levk/core/not_null.hpp>
#include <levk/core/ptr.hpp>
#include <levk/core/version.hpp>
#include <levk/defer_queue.hpp>
#include <levk/gpu_selector.hpp>
#include <levk/pipeline.hpp>
#include <levk/pipeline_state.hpp>
#include <levk/primitive.hpp>
#include <levk/render_camera.hpp>
#include <levk/render_resource.hpp>
#include <levk/render_target.hpp>
#include <levk/shader_buffer.hpp>
#include <levk/texture.hpp>
#include <levk/vertex_buffer.hpp>
#include <memory>

namespace levk {
struct RenderDeviceCreateInfo {
	NotNull<GLFWwindow*> window;
	CString app_name{"App"};
	Version app_version{};
	Ptr<IGpuSelector> gpu_selector{};
};

/// \brief Abstract class representing the Render Device.
class IRenderDevice : public Polymorphic {
  public:
	[[nodiscard]] virtual auto get_instance() const -> vk::Instance = 0;
	[[nodiscard]] virtual auto get_physical_device() const -> vk::PhysicalDevice = 0;
	[[nodiscard]] virtual auto get_surface() const -> vk::SurfaceKHR = 0;
	[[nodiscard]] virtual auto get_device() const -> vk::Device = 0;
	[[nodiscard]] virtual auto get_queue_family() const -> std::uint32_t = 0;
	[[nodiscard]] virtual auto get_allocator() const -> VmaAllocator = 0;
	[[nodiscard]] virtual auto get_properties() const -> vk::PhysicalDeviceProperties const& = 0;

	[[nodiscard]] virtual auto get_defer_queue() const -> IDeferQueue& = 0;

	[[nodiscard]] virtual auto get_frame_index() const -> FrameIndex = 0;
	[[nodiscard]] virtual auto get_swapchain_extent() const -> vk::Extent2D = 0;
	[[nodiscard]] virtual auto get_swapchain_format() const -> vk::Format = 0;
	[[nodiscard]] virtual auto get_depth_format() const -> vk::Format = 0;

	[[nodiscard]] virtual auto create_image(ImageCreateInfo const& create_info) -> std::unique_ptr<IRenderImage> = 0;
	[[nodiscard]] virtual auto create_buffer(BufferCreateInfo const& create_info) -> std::unique_ptr<IRenderBuffer> = 0;
	[[nodiscard]] virtual auto create_static_vertex_buffer(VertexArray const& vertices) -> std::unique_ptr<IStaticVertexBuffer> = 0;
	[[nodiscard]] virtual auto create_dynamic_vertex_buffer(VertexArray vertices = {}) -> std::unique_ptr<IDynamicVertexBuffer> = 0;
	[[nodiscard]] virtual auto create_skin_vertex_buffer(VertexSkin const& skin) -> std::unique_ptr<IStaticVertexBuffer> = 0;
	[[nodiscard]] virtual auto create_uniform_buffer(vk::DeviceSize size = {}) -> std::unique_ptr<IUniformBuffer> = 0;
	[[nodiscard]] virtual auto create_storage_buffer(vk::DeviceSize size = {}) -> std::unique_ptr<IStorageBuffer> = 0;
	[[nodiscard]] virtual auto create_texture(BitmapView bitmap, bool mip_map = true, bool linear = false) -> std::unique_ptr<ITexture> = 0;
	[[nodiscard]] virtual auto create_dynamic_texture(BitmapView bitmap = {}, bool mip_map = true, bool linear = false) -> std::unique_ptr<IDynamicTexture> = 0;
	[[nodiscard]] virtual auto create_cubemap(std::span<BitmapView const, 6> layers, bool linear = false) -> std::unique_ptr<ICubemap> = 0;
	[[nodiscard]] virtual auto create_static_primitive(Geometry const& geometry, NotNull<IMaterial const*> material,
													   RenderShader vertex_shader) -> std::unique_ptr<IStaticPrimitive> = 0;
	[[nodiscard]] virtual auto create_dynamic_primitive(Geometry geometry, NotNull<IMaterial const*> material,
														RenderShader vertex_shader) -> std::unique_ptr<IDynamicPrimitive> = 0;
	[[nodiscard]] virtual auto create_skinned_primitive(Geometry const& geometry, NotNull<IMaterial const*> material,
														RenderShader vertex_shader) -> std::unique_ptr<ISkinnedPrimitive> = 0;

	[[nodiscard]] virtual auto create_3d_camera(vk::SampleCountFlagBits samples) -> std::unique_ptr<IRenderCamera> = 0;
	[[nodiscard]] virtual auto create_2d_camera(vk::SampleCountFlagBits samples) -> std::unique_ptr<IRenderCamera> = 0;
	[[nodiscard]] virtual auto create_depth_only_camera(vk::SampleCountFlagBits samples) -> std::unique_ptr<IRenderCamera> = 0;

	[[nodiscard]] virtual auto get_fallback_texture() const -> ITexture const& = 0;
	[[nodiscard]] virtual auto get_fallback_texture_black() const -> ITexture const& = 0;
	[[nodiscard]] virtual auto get_pipeline(RenderShader vertex, RenderShader fragment, PipelineState const& state) -> Ptr<IPipeline> = 0;
	[[nodiscard]] virtual auto get_sampler(TextureSampler const& texture_sampler) -> vk::Sampler = 0;
	[[nodiscard]] virtual auto allocate_storage_buffer() -> IStorageBuffer& = 0;

	virtual void queue_submit(vk::SubmitInfo2 const& submit_info, vk::Fence signal = {}) = 0;

	virtual void new_frame() = 0;
	virtual auto acquire_next_image() -> vk::CommandBuffer = 0;
	virtual void submit_and_present(RenderTarget const& render_target) = 0;

	bool render_dear_imgui{true};
};

/// \brief Create a concrete Render Device.
auto create_render_device(RenderDeviceCreateInfo const& create_info) noexcept(false) -> std::unique_ptr<IRenderDevice>;

constexpr auto to_vsync_string(vk::PresentModeKHR const mode) -> std::string_view {
	switch (mode) {
	case vk::PresentModeKHR::eFifo: return "classic";
	case vk::PresentModeKHR::eFifoRelaxed: return "adaptive";
	case vk::PresentModeKHR::eMailbox: return "mailbox";
	case vk::PresentModeKHR::eImmediate: return "immediate";
	default: return "unsupported";
	}
}
} // namespace levk
