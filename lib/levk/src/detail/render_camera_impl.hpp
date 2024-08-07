#pragma once
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/matrix_transform.hpp>	 // glm::translate, glm::rotate, glm::scale
#include <levk/dynamic_renderer.hpp>
#include <levk/logger.hpp>
#include <levk/render_camera.hpp>
#include <levk/render_device.hpp>
#include <levk/transform.hpp>
#include <levk/util.hpp>

namespace levk {
class Framebuffer {
  public:
	enum Flag : int {
		eCreateColour = 1 << 0,
		eCreateDepth = 1 << 1,
	};

	using Image = RenderTarget::Image;

	explicit Framebuffer(NotNull<IRenderDevice*> device, vk::SampleCountFlagBits const samples, int const flags) : m_device(device), m_samples(samples) {
		if ((flags & eCreateColour) == eCreateColour) { create_colour(samples); }
		if ((flags & eCreateDepth) == eCreateDepth) { create_depth(samples); }
		set_extent({1, 1});
	}

	static auto with_colour(NotNull<IRenderDevice*> device, vk::SampleCountFlagBits const samples) -> Framebuffer {
		return Framebuffer{device, samples, eCreateColour};
	}

	static auto with_depth(NotNull<IRenderDevice*> device, vk::SampleCountFlagBits const samples) -> Framebuffer {
		return Framebuffer{device, samples, eCreateDepth};
	}

	static auto with_colour_and_depth(NotNull<IRenderDevice*> device, vk::SampleCountFlagBits const samples) -> Framebuffer {
		return Framebuffer{device, samples, eCreateColour | eCreateDepth};
	}

	[[nodiscard]] auto get_render_target() const -> RenderTarget const& { return m_target; }

	void set_attachments(vk::Extent2D extent, Image const& colour, Image const& depth, Image const& resolve) {
		set_extent(extent);
		set_colour(colour);
		set_depth(depth);
		set_resolve(resolve);
	}

	[[nodiscard]] auto has_depth_attachment() const -> bool { return !!m_target.depth.view; }
	[[nodiscard]] auto get_sample_count() const -> vk::SampleCountFlagBits { return m_samples; }

	[[nodiscard]] auto resize(vk::Extent2D extent, float const scale) -> bool {
		if (!m_colour && !m_depth) { return false; }
		extent = safe_image_extent(scaled_extent(extent, scale));
		set_extent(extent);
		if (m_colour) { m_colour->resize(extent); }
		if (m_depth) { m_depth->resize(extent); }
		if (m_resolve) { m_resolve->resize(extent); }
		set_owned_attachments();
		return true;
	}

  private:
	struct CreateInfo {
		vk::ImageAspectFlagBits aspect{vk::ImageAspectFlagBits::eColor};
		vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
		vk::Format format{vk::Format::eR8G8B8A8Srgb};
	};

	[[nodiscard]] static auto to_rt_image(IRenderImage const& image) -> Image {
		auto const& info = image.get_image_info();
		return Image{.image = info.image, .view = info.view, .format = info.format};
	}

	[[nodiscard]] auto create_image(CreateInfo const& create_info) const -> std::unique_ptr<IRenderImage> {
		auto const common_ici = ImageCreateInfo{
			.extent = m_target.extent,
			.format = create_info.format,
			.aspect = create_info.aspect,
			.samples = create_info.samples,
			.mip_map = false,
		};
		if (create_info.aspect == vk::ImageAspectFlagBits::eDepth) {
			auto depth_ici = common_ici;
			depth_ici.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
			return m_device->create_image(depth_ici);
		}
		auto colour_ici = common_ici;
		colour_ici.usage |= vk::ImageUsageFlagBits::eColorAttachment;
		return m_device->create_image(colour_ici);
	}

	void create_colour(vk::SampleCountFlagBits const samples) {
		m_colour = create_image(CreateInfo{.samples = samples});
		if (samples > vk::SampleCountFlagBits::e1) { create_resolve(); }
	}

	void create_resolve() { m_resolve = create_image({}); }

	void create_depth(vk::SampleCountFlagBits const samples) {
		auto const create_info = CreateInfo{
			.aspect = vk::ImageAspectFlagBits::eDepth,
			.samples = samples,
			.format = m_device->get_depth_format(),
		};
		m_depth = create_image(create_info);
	}

	void set_extent(vk::Extent2D extent) { m_target.extent = extent; }
	void set_colour(Image const& image) { m_target.colour = image; }
	void set_resolve(Image const& image) { m_target.resolve = image; }
	void set_depth(Image const& image) { m_target.depth = image; }

	void set_owned_attachments() {
		if (m_colour) { set_colour(to_rt_image(*m_colour)); }
		if (m_depth) { set_depth(to_rt_image(*m_depth)); }
		if (m_resolve) { set_resolve(to_rt_image(*m_resolve)); }
	}

	NotNull<IRenderDevice*> m_device;
	vk::SampleCountFlagBits m_samples{};

	std::unique_ptr<IRenderImage> m_colour{};
	std::unique_ptr<IRenderImage> m_depth{};
	std::unique_ptr<IRenderImage> m_resolve{};

	RenderTarget m_target{};
};

class RenderTexture : public IRenderTexture {
  public:
	explicit RenderTexture(IRenderDevice& device) : m_fallback(&device.get_fallback_texture()) {}

	RenderTarget render_target{};
	mutable vk::Sampler sampler{};

  private:
	[[nodiscard]] auto get_size() const -> glm::ivec2 final {
		auto const ret = to_glm_vec2(render_target.extent);
		if (!is_positive(ret)) { return m_fallback->get_size(); }
		return ret;
	}

	[[nodiscard]] auto get_descriptor_info(std::uint32_t const binding) const -> DescriptorInfo final {
		auto const image = render_target.get_output();
		if (!image.view) { return m_fallback->get_descriptor_info(binding); }

		return DescriptorInfo{
			.payload = vk::DescriptorImageInfo{sampler, image.view, vk::ImageLayout::eShaderReadOnlyOptimal},
			.type = vk::DescriptorType::eCombinedImageSampler,
			.binding = binding,
		};
	}

	NotNull<ITexture const*> m_fallback;
};

class RenderCamera : public IRenderCamera {
  public:
	explicit RenderCamera(NotNull<IRenderDevice*> device, Framebuffer framebuffer, DynamicRenderer::StoreOp depth_store)
		: m_device(device), m_framebuffer(std::move(framebuffer)), m_renderer(device), m_render_texture(*device), m_view_proj(device->create_uniform_buffer()),
		  m_main_light(device->create_uniform_buffer()) {
		m_renderer.depth_store_op = depth_store;
	}

  private:
	void begin_rendering(RenderBeginInfo const& info, glm::ivec2 const resolution, vk::CommandBuffer const command_buffer) final {
		if (m_command_buffer) {
			m_log.warn("RenderCamera::begin_rendering() duplicate call without end_rendering() in between");
			return;
		}

		m_renderer.clear_colour = clear_colour;
		if (!m_framebuffer.resize(to_vk_extent(resolution), render_scale)) {
			m_log.warn("RenderCamera::begin_rendering(): failed to resize framebuffer");
			return;
		}

		m_render_target = m_framebuffer.get_render_target();
		m_render_texture.render_target = {};
		m_renderer.begin_rendering(m_render_target, command_buffer);
		m_command_buffer = command_buffer;

		write_view_proj(info);
		write_main_light(info.main_light);
	}

	[[nodiscard]] auto get_render_state() const -> RenderPassState final {
		return RenderPassState{
			.polygon_mode = polygon_mode,
			.colour_format = m_render_target.colour.format,
			.depth_format = m_render_target.depth.format,
			.samples = m_framebuffer.get_sample_count(),
			.depth_test = m_framebuffer.has_depth_attachment(),
		};
	}

	[[nodiscard]] auto get_view_buffer() const -> IUniformBuffer const& final { return *m_view_proj; }
	[[nodiscard]] auto get_main_light_buffer() const -> IUniformBuffer const& final { return *m_main_light; }
	[[nodiscard]] auto get_render_target() const -> RenderTarget const& final { return m_render_target; }

	[[nodiscard]] auto end_rendering() -> RenderTarget final {
		if (!m_command_buffer) { return {}; }
		auto const ret = m_render_target;
		m_command_buffer = vk::CommandBuffer{};
		m_render_texture.render_target = std::exchange(m_render_target, {});
		m_renderer.end_rendering();
		return ret;
	}

	[[nodiscard]] auto get_render_texture(TextureSampler const& sampler) const -> IRenderTexture const& final {
		m_render_texture.sampler = m_device->get_sampler(sampler);
		return m_render_texture;
	}

	void write_view_proj(RenderBeginInfo const& info) {
		struct ViewProj {
			glm::mat4 view_proj;
			glm::mat4 view;
			glm::mat4 proj;
			glm::vec4 campos_exposure;
			glm::mat4 shadow_view_proj;
		};
		auto view_proj = ViewProj{
			.view_proj = info.camera_proj * info.camera_view,
			.view = info.camera_view,
			.proj = info.camera_proj,
			.campos_exposure = {info.camera_position, info.camera_exposure},
			.shadow_view_proj = info.shadow_view_proj,
		};
		m_view_proj->set_data(&view_proj, sizeof(view_proj));
		m_bound_view_sets.clear();
	}

	void write_main_light(DirectionalLight const& main_light) {
		struct DirLight {
			glm::vec4 rgb_intensity;
			alignas(16) glm::vec3 direction;
		};
		auto const rgba = RgbF32{colour::to_f32(main_light.tint)};
		auto const intensity = main_light.intensity;
		auto const dir_light = DirLight{
			.rgb_intensity = {rgba, intensity},
			.direction = glm::normalize(main_light.orientation * front_v),
		};
		m_main_light->set_data(&dir_light, sizeof(dir_light));
	}

	Logger m_log{"RenderCamera"};

	NotNull<IRenderDevice*> m_device;

	Framebuffer m_framebuffer;
	DynamicRenderer m_renderer;
	RenderTexture m_render_texture;

	std::unique_ptr<levk::IUniformBuffer> m_view_proj{};
	std::unique_ptr<levk::IUniformBuffer> m_main_light{};
	std::vector<vk::WriteDescriptorSet> m_write_set_buffer{};

	vk::CommandBuffer m_command_buffer{};
	RenderTarget m_render_target{};
	std::unordered_map<Ptr<IPipeline>, vk::DescriptorSet> m_bound_view_sets{};
};
} // namespace levk
