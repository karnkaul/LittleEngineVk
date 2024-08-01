#include <detail/window_helper.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <detail/pipeline_cache.hpp>
#include <detail/primitive_impl.hpp>
#include <detail/render_camera_impl.hpp>
#include <detail/render_resource_impl.hpp>
#include <detail/shader_buffer_impl.hpp>
#include <detail/texture_impl.hpp>
#include <detail/vertex_buffer_impl.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/mat4x4.hpp>
#include <levk/build_version.hpp>
#include <levk/command_buffer.hpp>
#include <levk/core/error.hpp>
#include <levk/core/hash_combine.hpp>
#include <levk/core/is_positive.hpp>
#include <levk/logger.hpp>
#include <levk/render_device.hpp>

namespace levk {
using namespace std::chrono_literals;

namespace {
constexpr std::array device_extensions_v = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(__APPLE__)
											VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
};

constexpr auto srgb_formats_v = std::array{vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb, vk::Format::eA8B8G8R8SrgbPack32};

constexpr auto has_required_extensions(std::span<vk::ExtensionProperties const> available) {
	auto const has_extension = [available](char const* name) {
		auto const match = [name](vk::ExtensionProperties const& props) { return std::string_view{props.extensionName.data()} == name; };
		return std::ranges::find_if(available, match) != available.end();
	};
	return std::ranges::all_of(device_extensions_v, has_extension);
}

auto find_best_depth_format(vk::PhysicalDevice const gpu) -> vk::Format {
	static constexpr auto target{vk::Format::eD32Sfloat};
	auto const props = gpu.getFormatProperties(target);
	if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) { return target; }
	return vk::Format::eD16Unorm;
}

[[nodiscard]] auto create_instance(RenderDeviceCreateInfo const& create_info) {
	auto vdl = vk::DynamicLoader{};
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vdl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	auto extensions = detail::get_instance_extensions();
	auto ici = vk::InstanceCreateInfo{};
#if defined(__APPLE__)
	ici.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
	ici.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
	ici.ppEnabledExtensionNames = extensions.data();
	auto ai = vk::ApplicationInfo{};
	ai.apiVersion = VK_API_VERSION_1_3;
	ai.engineVersion = VK_MAKE_VERSION(version_v.major, version_v.minor, version_v.patch);
	ai.applicationVersion = VK_MAKE_VERSION(create_info.app_version.major, create_info.app_version.minor, create_info.app_version.patch);
	ai.pEngineName = "levk";
	ai.pApplicationName = create_info.app_name.c_str();
	ici.pApplicationInfo = &ai;
	auto ret = vk::createInstanceUnique(ici);
	if (!ret) { throw Error{"Failed to create Vulkan Instance"}; }
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*ret);
	return ret;
}

struct Gpu {
	vk::PhysicalDevice device{};
	std::uint32_t queue_family{};

	[[nodiscard]] static auto get_viable(vk::Instance instance, vk::SurfaceKHR surface) {
		static constexpr auto queue_flags_v = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
		auto const all_devices = instance.enumeratePhysicalDevices();
		auto ret = std::vector<Gpu>{};
		auto const get_queue_family = [](vk::PhysicalDevice device, std::uint32_t& out) {
			for (auto const& [index, family] : std::ranges::enumerate_view(device.getQueueFamilyProperties())) {
				if ((family.queueFlags & queue_flags_v) != queue_flags_v) { continue; }
				out = static_cast<std::uint32_t>(index);
				return true;
			}
			return false;
		};
		for (auto const& device : all_devices) {
			auto gpu = Gpu{.device = device};
			if (device.getProperties().apiVersion < VK_API_VERSION_1_3) { continue; }
			if (!has_required_extensions(device.enumerateDeviceExtensionProperties())) { continue; }
			if (!get_queue_family(device, gpu.queue_family)) { continue; }
			if (device.getSurfaceSupportKHR(gpu.queue_family, surface) == vk::False) { continue; }
			ret.push_back(gpu);
		}
		return ret;
	}
};

struct GpuSelector : IGpuSelector {
	Ptr<IGpuSelector> user{};

	explicit GpuSelector(Ptr<IGpuSelector> user) : user(user) {}

	static auto select_discrete(std::span<vk::PhysicalDevice const> devices) {
		for (auto const& device : devices) {
			if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) { return device; }
		}
		return devices.front();
	}

	auto select_device(std::span<vk::PhysicalDevice const> devices) -> vk::PhysicalDevice final {
		auto ret = vk::PhysicalDevice{};
		if (user != nullptr) { ret = user->select_device(devices); }
		if (!ret) { ret = select_discrete(devices); }
		return ret;
	}
};

struct Swapchain {
	struct Sync {
		vk::UniqueSemaphore draw{};
		vk::UniqueSemaphore present{};
		vk::UniqueFence drawn{};
	};

	std::vector<vk::PresentModeKHR> present_modes{};

	vk::SwapchainCreateInfoKHR info{};

	vk::UniqueSwapchainKHR active{};
	vk::UniqueCommandPool command_pool{};
	std::vector<vk::Image> images{};
	std::vector<vk::UniqueImageView> views{};

	std::optional<std::uint32_t> image_index{};
	Buffered<Sync> syncs{};
	Buffered<vk::CommandBuffer> command_buffers{};

	[[nodiscard]] auto get_optimal_present_mode() const {
		static constexpr auto desired_v = std::array{vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifoRelaxed};
		for (auto const desired : desired_v) {
			if (std::ranges::find(present_modes, desired) != present_modes.end()) { return desired; }
		}
		return vk::PresentModeKHR::eFifo;
	}

	[[nodiscard]] static constexpr auto get_surface_format(std::span<vk::SurfaceFormatKHR const> supported) {
		for (auto const srgb_format : srgb_formats_v) {
			auto const it = std::ranges::find_if(supported, [srgb_format](vk::SurfaceFormatKHR const& format) {
				return format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && format.format == srgb_format;
			});
			if (it != supported.end()) { return *it; }
		}
		return vk::SurfaceFormatKHR{};
	}

	[[nodiscard]] static constexpr auto get_composite_alpha(vk::SurfaceCapabilitiesKHR const& caps) {
		using enum vk::CompositeAlphaFlagBitsKHR;
		if (caps.supportedCompositeAlpha & eOpaque) { return eOpaque; }
		if (caps.supportedCompositeAlpha & eInherit) { return eInherit; }
		if (caps.supportedCompositeAlpha & ePreMultiplied) { return ePreMultiplied; }
		// according to the spec, at least one bit must be set
		return ePostMultiplied;
	}

	[[nodiscard]] static auto get_image_extent(vk::SurfaceCapabilitiesKHR const& caps, vk::Extent2D framebuffer) {
		static constexpr auto limitless_v = std::numeric_limits<std::uint32_t>::max();
		if (caps.currentExtent.width < limitless_v && caps.currentExtent.height < limitless_v) { return caps.currentExtent; }
		auto const x = std::clamp(framebuffer.width, caps.minImageExtent.width, caps.maxImageExtent.width);
		auto const y = std::clamp(framebuffer.height, caps.minImageExtent.height, caps.maxImageExtent.height);
		return vk::Extent2D{x, y};
	}

	[[nodiscard]] static constexpr auto get_image_count(vk::SurfaceCapabilitiesKHR const& caps) {
		if (caps.maxImageCount < caps.minImageCount) { return std::max(3u, caps.minImageCount + 1); }
		return std::clamp(3u, caps.minImageCount + 1, caps.maxImageCount);
	}
};

class DeferQueue : public IDeferQueue {
  public:
	explicit DeferQueue(NotNull<IRenderDevice const*> device) : m_device(device) {}

	void next_frame() {
		auto lock = std::scoped_lock{m_mutex};
		auto& frame = m_frames.front();
		frame.ts.clear();
		for (auto& callback : frame.callbacks) { callback(); }
		frame.callbacks.clear();
		std::ranges::rotate(m_frames, m_frames.begin() + 1);
	}

	void clear() {
		auto lock = std::scoped_lock{m_mutex};
		for (auto& frame : m_frames) {
			frame.ts.clear();
			for (auto& callback : frame.callbacks) { callback(); }
			frame.callbacks.clear();
		}
	}

  private:
	struct Frame {
		std::vector<std::unique_ptr<Base>> ts{};
		std::vector<std::move_only_function<void()>> callbacks{};
	};

	void do_push(std::unique_ptr<Base> t) final {
		auto lock = std::scoped_lock{m_mutex};
		m_frames.back().ts.push_back(std::move(t));
	}

	void do_push(std::move_only_function<void()> f) final {
		auto lock = std::scoped_lock{m_mutex};
		m_frames.back().callbacks.push_back(std::move(f));
	}

	NotNull<IRenderDevice const*> m_device;
	std::mutex m_mutex{};
	std::array<Frame, buffering_v + 1> m_frames{};
};

class SamplerCache {
  public:
	explicit SamplerCache(vk::Device device, vk::PhysicalDeviceProperties const& properties)
		: m_device(device), m_max_anisotropy(properties.limits.maxSamplerAnisotropy) {}

	auto get_or_create(TextureSampler const& sampler) -> vk::Sampler {
		if (auto it = m_map.find(sampler); it != m_map.end()) { return *it->second; }
		auto const anisotropy = std::min(sampler.anisotropy, m_max_anisotropy);
		auto sci = vk::SamplerCreateInfo{};
		sci.addressModeU = sampler.wrap_u;
		sci.addressModeV = sampler.wrap_v;
		sci.addressModeW = sampler.wrap_w;
		sci.minFilter = sampler.min_filter;
		sci.magFilter = sampler.mag_filter;
		sci.anisotropyEnable = anisotropy > 0.0f ? vk::True : vk::False;
		sci.maxAnisotropy = anisotropy;
		sci.borderColor = sampler.border;
		sci.mipmapMode = vk::SamplerMipmapMode::eNearest;
		sci.maxLod = VK_LOD_CLAMP_NONE;
		auto const [it, _] = m_map.insert_or_assign(sampler, m_device.createSamplerUnique(sci));
		return *it->second;
	}

  private:
	struct Hasher {
		auto operator()(TextureSampler const& sampler) const -> std::size_t {
			return make_combined_hash(sampler.wrap_u, sampler.wrap_v, sampler.wrap_w, sampler.min_filter, sampler.mag_filter, sampler.border);
		}
	};

	vk::Device m_device{};
	float m_max_anisotropy{};

	std::unordered_map<TextureSampler, vk::UniqueSampler, Hasher> m_map{};
};

class TextureCache {
  public:
	explicit TextureCache(NotNull<IRenderDevice*> device) : m_device(device) {
		m_white = create(colour::white_v);
		m_black = create(colour::black_v);
	}

	[[nodiscard]] auto get(bool const black) const -> ITexture const& {
		if (black) { return *m_black; }
		return *m_white;
	}

  private:
	[[nodiscard]] auto create(RgbaU8 const& rgba) const -> std::unique_ptr<ITexture> {
		auto bytes = std::array<std::byte, sizeof(RgbaU8)>{};
		std::memcpy(bytes.data(), &rgba, sizeof(RgbaU8));
		auto const bitmap = BitmapView{.bytes = bytes, .extent = {1, 1}};
		return m_device->create_texture(bitmap, false);
	}

	NotNull<IRenderDevice*> m_device;

	std::unique_ptr<ITexture> m_white{};
	std::unique_ptr<ITexture> m_black{};
};

class StorageBufferPool {
  public:
	explicit StorageBufferPool(NotNull<IRenderDevice*> device) : m_device(device) {}

	[[nodiscard]] auto allocate() -> IStorageBuffer& {
		static constexpr auto bloat_v = std::size_t{10000};
		if (m_buffers.size() > bloat_v) { m_log.warn("'{}' instance buffers already allocated, possible leak?", m_buffers.size()); }
		if (m_next >= m_buffers.size()) { m_buffers.push_back(m_device->create_storage_buffer()); }
		return *m_buffers.at(m_next++);
	}

	void next_frame() { m_next = 0; }

  private:
	Logger m_log{"InstanceBufferCache"};

	NotNull<IRenderDevice*> m_device;

	std::vector<std::unique_ptr<IStorageBuffer>> m_buffers{};
	std::size_t m_next{};
};

class DearImGui {
  public:
	struct CreateInfo { // NOLINT(cppcoreguidelines-pro-type-member-init)
		NotNull<GLFWwindow*> window;
		vk::Queue queue;
		vk::SampleCountFlagBits samples;
		vk::Format colour;
	};

	DearImGui(DearImGui const&) = delete;
	DearImGui(DearImGui&&) = delete;
	auto operator=(DearImGui const&) = delete;
	auto operator=(DearImGui&&) = delete;

	DearImGui(IRenderDevice& render_device, CreateInfo const& create_info) : m_device(render_device.get_device()) {
		static constexpr std::uint32_t max_textures_v{16};
		auto const pool_sizes = std::array{
			vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, max_textures_v},
		};
		auto dpci = vk::DescriptorPoolCreateInfo{};
		dpci.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
		dpci.maxSets = max_textures_v;
		dpci.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
		dpci.pPoolSizes = pool_sizes.data();
		m_pool = render_device.get_device().createDescriptorPoolUnique(dpci);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

		ImGui::StyleColorsDark();
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
		for (auto& colour : ImGui::GetStyle().Colors) {
			auto const corrected = glm::convertSRGBToLinear(glm::vec4{colour.x, colour.y, colour.z, colour.w});
			colour = {corrected.x, corrected.y, corrected.z, corrected.w};
		}

		auto loader = vk::DynamicLoader{};
		auto get_fn = [&loader](char const* name) { return loader.getProcAddress<PFN_vkVoidFunction>(name); };
		auto lambda = +[](char const* name, void* ud) {
			if (std::string_view{name} == "vkCmdBeginRenderingKHR") { name = "vkCmdBeginRendering"; }
			if (std::string_view{name} == "vkCmdEndRenderingKHR") { name = "vkCmdEndRendering"; }
			auto const* gf = reinterpret_cast<decltype(get_fn)*>(ud); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
			return (*gf)(name);
		};
		ImGui_ImplVulkan_LoadFunctions(lambda, &get_fn);
		ImGui_ImplGlfw_InitForVulkan(create_info.window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = render_device.get_instance();
		init_info.PhysicalDevice = render_device.get_physical_device();
		init_info.Device = render_device.get_device();
		init_info.QueueFamily = render_device.get_queue_family();
		init_info.Queue = create_info.queue;
		init_info.DescriptorPool = *m_pool;
		init_info.Subpass = 0;
		init_info.MinImageCount = 2;
		init_info.ImageCount = buffering_v + 1;
		init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(create_info.samples);
		init_info.ColorAttachmentFormat = static_cast<VkFormat>(create_info.colour);
		init_info.UseDynamicRendering = true;

		ImGui_ImplVulkan_Init(&init_info, {});

		auto command_buffer = CommandBuffer{render_device};
		ImGui_ImplVulkan_CreateFontsTexture(command_buffer.get());
		command_buffer.submit(render_device);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	~DearImGui() {
		m_device.waitIdle();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void new_frame() { // NOLINT(misc-no-recursion)
		if (m_state == State::eEndFrame) { end_frame(); }
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		m_state = State::eEndFrame;
	}

	void end_frame() { // NOLINT(misc-no-recursion)
		// ImGui::Render calls ImGui::EndFrame
		if (m_state == State::eNewFrame) { new_frame(); }
		ImGui::Render();
		m_state = State::eNewFrame;
	}

	void render(vk::CommandBuffer const command_buffer) {
		if (m_state == State::eEndFrame) { end_frame(); }
		if (auto* data = ImGui::GetDrawData()) { ImGui_ImplVulkan_RenderDrawData(data, command_buffer); }
	}

	enum class State { eNewFrame, eEndFrame };

	vk::Device m_device{};

	vk::UniqueDescriptorPool m_pool{};
	State m_state{};
};

class RenderDevice : public IRenderDevice {
  public:
	using CreateInfo = RenderDeviceCreateInfo;

	RenderDevice(RenderDevice const&) = delete;
	RenderDevice(RenderDevice&&) = delete;
	auto operator=(RenderDevice const&) = delete;
	auto operator=(RenderDevice&&) = delete;

	explicit RenderDevice(CreateInfo const& create_info) : m_window(create_info.window), m_defer_queue(std::make_unique<DeferQueue>(this)) {
		m_instance = create_instance(create_info);
		m_surface = detail::create_surface(m_window, *m_instance);
		select_physical_device(create_info);
		create_device();
		create_swapchain();
		create_allocator_and_caches();
		create_dear_imgui();
	}

	~RenderDevice() override {
		try {
			m_device->waitIdle();
		} catch (vk::SystemError const& e) { m_log.error("Vulkan error on Device.waitIdle(): {}", e.what()); }
		m_texture_cache.reset();
		m_storage_buffer_pool.reset();
		m_pipeline_cache.reset();
		m_descriptor_allocator.reset();
		m_defer_queue->clear();
		vmaDestroyAllocator(m_allocator);
	}

  private:
	[[nodiscard]] auto get_instance() const -> vk::Instance final { return *m_instance; }
	[[nodiscard]] auto get_physical_device() const -> vk::PhysicalDevice final { return m_physical_device; }
	[[nodiscard]] auto get_surface() const -> vk::SurfaceKHR final { return *m_surface; }
	[[nodiscard]] auto get_device() const -> vk::Device final { return *m_device; }
	[[nodiscard]] auto get_queue_family() const -> std::uint32_t final { return m_queue_family; }
	[[nodiscard]] auto get_allocator() const -> VmaAllocator final { return m_allocator; }
	[[nodiscard]] auto get_properties() const -> vk::PhysicalDeviceProperties const& final { return m_properties; }

	[[nodiscard]] auto get_defer_queue() const -> IDeferQueue& final { return *m_defer_queue; }

	[[nodiscard]] auto get_frame_index() const -> FrameIndex final { return m_frame_index; }
	[[nodiscard]] auto get_swapchain_extent() const -> vk::Extent2D final { return m_swapchain.info.imageExtent; }
	[[nodiscard]] auto get_swapchain_format() const -> vk::Format final { return m_swapchain.info.imageFormat; }
	[[nodiscard]] auto get_depth_format() const -> vk::Format final { return m_depth_format; }

	auto create_image(ImageCreateInfo const& create_info) -> std::unique_ptr<IRenderImage> final { return std::make_unique<RenderImage>(this, create_info); }

	auto create_buffer(BufferCreateInfo const& create_info) -> std::unique_ptr<IRenderBuffer> final {
		return std::make_unique<RenderBuffer>(this, create_info);
	}

	auto create_static_vertex_buffer(VertexArray const& vertices) -> std::unique_ptr<IStaticVertexBuffer> final {
		return std::make_unique<StaticVertexBuffer>(this, vertices);
	}

	auto create_dynamic_vertex_buffer(VertexArray vertices) -> std::unique_ptr<IDynamicVertexBuffer> final {
		return std::make_unique<DynamicVertexBuffer>(this, std::move(vertices));
	}

	auto create_skin_vertex_buffer(VertexSkin const& skin) -> std::unique_ptr<IStaticVertexBuffer> final {
		return std::make_unique<StaticVertexBuffer>(this, skin);
	}

	auto create_uniform_buffer(vk::DeviceSize const size) -> std::unique_ptr<IUniformBuffer> final {
		return std::make_unique<ShaderBuffer<IUniformBuffer>>(this, size);
	}

	auto create_storage_buffer(vk::DeviceSize const size) -> std::unique_ptr<IStorageBuffer> final {
		return std::make_unique<ShaderBuffer<IStorageBuffer>>(this, size);
	}

	auto create_texture(BitmapView const bitmap, bool const mip_map, bool const linear) -> std::unique_ptr<ITexture> final {
		return std::make_unique<Texture>(this, bitmap, mip_map, linear);
	}

	auto create_dynamic_texture(BitmapView bitmap = {}, bool mip_map = true, bool linear = false) -> std::unique_ptr<IDynamicTexture> final {
		return std::make_unique<DynamicTexture>(this, bitmap, mip_map, linear);
	}

	auto create_cubemap(std::span<BitmapView const, 6> layers, bool const linear) -> std::unique_ptr<ICubemap> final {
		return std::make_unique<Cubemap>(this, layers, linear);
	}

	auto create_static_primitive(Geometry const& geometry, NotNull<IMaterial const*> material,
								 RenderShader vertex_shader) -> std::unique_ptr<IStaticPrimitive> final {
		return std::make_unique<StaticPrimitive>(this, geometry, material, vertex_shader);
	}

	auto create_dynamic_primitive(Geometry geometry, NotNull<IMaterial const*> material,
								  RenderShader vertex_shader) -> std::unique_ptr<IDynamicPrimitive> final {
		return std::make_unique<DynamicPrimitive>(this, std::move(geometry), material, vertex_shader);
	}

	auto create_skinned_primitive(Geometry const& geometry, NotNull<IMaterial const*> material,
								  RenderShader vertex_shader) -> std::unique_ptr<ISkinnedPrimitive> final {
		return std::make_unique<SkinnedPrimitive>(this, geometry, material, vertex_shader);
	}

	auto create_3d_camera(vk::SampleCountFlagBits const samples) -> std::unique_ptr<IRenderCamera> final {
		auto framebuffer = Framebuffer::with_colour_and_depth(this, samples);
		return std::make_unique<RenderCamera>(this, std::move(framebuffer), DynamicRenderer::StoreOp::eDontCare);
	}

	auto create_2d_camera(vk::SampleCountFlagBits const samples) -> std::unique_ptr<IRenderCamera> final {
		auto framebuffer = Framebuffer::with_colour(this, samples);
		return std::make_unique<RenderCamera>(this, std::move(framebuffer), DynamicRenderer::StoreOp::eDontCare);
	}

	auto create_depth_only_camera(vk::SampleCountFlagBits const samples) -> std::unique_ptr<IRenderCamera> final {
		auto framebuffer = Framebuffer::with_depth(this, samples);
		return std::make_unique<RenderCamera>(this, std::move(framebuffer), DynamicRenderer::StoreOp::eStore);
	}

	[[nodiscard]] auto get_fallback_texture() const -> ITexture const& final { return m_texture_cache->get(false); }
	[[nodiscard]] auto get_fallback_texture_black() const -> ITexture const& final { return m_texture_cache->get(true); }

	auto get_pipeline(RenderShader const vertex, RenderShader const fragment, PipelineState const& state) -> Ptr<IPipeline> final {
		return m_pipeline_cache->get_or_create(vertex, fragment, state);
	}

	auto allocate_storage_buffer() -> IStorageBuffer& final { return m_storage_buffer_pool->allocate(); }

	auto get_sampler(TextureSampler const& sampler) -> vk::Sampler final { return m_sampler_cache->get_or_create(sampler); }

	void queue_submit(vk::SubmitInfo2 const& submit_info, vk::Fence const signal) final {
		auto lock = std::scoped_lock{m_queue_mutex};
		m_queue.submit2(submit_info, signal);
	}

	void new_frame() final { m_dear_imgui->new_frame(); }

	auto acquire_next_image() -> vk::CommandBuffer final {
		glm::uvec2 const framebuffer_size = detail::get_framebuffer_size(m_window);
		if (!is_positive(framebuffer_size)) { return {}; }

		if (framebuffer_size != to_glm_vec2(m_swapchain.info.imageExtent)) {
			if (!recreate_swapchain()) { return {}; }
		}

		static constexpr auto fence_timeout_v = std::chrono::duration<std::uint64_t, std::nano>{2s};
		static constexpr auto acquire_timeout_v = std::numeric_limits<std::uint64_t>::max();

		auto& sync = m_swapchain.syncs.at(m_frame_index);
		auto const wait_result = m_device->waitForFences(*sync.drawn, vk::True, fence_timeout_v.count());
		if (wait_result != vk::Result::eSuccess) { throw Error{"Failed to wait for drawn Vulkan Fence"}; }
		m_device->resetFences(*sync.drawn);
		m_defer_queue->next_frame();
		m_descriptor_allocator->next_frame();
		m_storage_buffer_pool->next_frame();

		auto image_index = std::uint32_t{};
		auto lock = std::unique_lock{m_queue_mutex};
		auto const acquire_result = m_device->acquireNextImageKHR(*m_swapchain.active, acquire_timeout_v, *sync.draw, vk::Fence{}, &image_index);
		lock.unlock();

		switch (acquire_result) {
		case vk::Result::eSuccess:
		case vk::Result::eSuboptimalKHR: break;
		case vk::Result::eErrorOutOfDateKHR: {
			recreate_swapchain();
			return {};
		}
		default: throw Error{"Failed to acquire next Vulkan Swapchain Image"};
		}

		// m_dear_imgui->new_frame();
		m_swapchain.image_index = image_index;

		auto const command_buffer = m_swapchain.command_buffers.at(m_frame_index);
		command_buffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

		return command_buffer;
	}

	void submit_and_present(RenderTarget const& render_target) final {
		auto& sync = m_swapchain.syncs.at(m_frame_index);
		auto const command_buffer = m_swapchain.command_buffers.at(m_frame_index);

		blit_to_swapchain(render_target, command_buffer);
		if (render_dear_imgui) { render_imgui(command_buffer); }

		command_buffer.end();

		auto const wait_ssi = vk::SemaphoreSubmitInfo{*sync.draw, 0, vk::PipelineStageFlagBits2::eAllCommands};
		auto const signal_ssi = vk::SemaphoreSubmitInfo{*sync.present, 0, vk::PipelineStageFlagBits2::eAllCommands};
		auto const cbi = vk::CommandBufferSubmitInfo{command_buffer};
		auto const si = vk::SubmitInfo2{
			vk::SubmitFlags{}, 1, &wait_ssi, 1, &cbi, 1, &signal_ssi,
		};

		auto pi = vk::PresentInfoKHR{};
		pi.pImageIndices = &*m_swapchain.image_index;
		pi.pSwapchains = &*m_swapchain.active;
		pi.swapchainCount = 1;
		pi.pWaitSemaphores = &*sync.present;
		pi.waitSemaphoreCount = 1;

		auto lock = std::unique_lock{m_queue_mutex};
		m_queue.submit2(si, *sync.drawn);
		auto const present_result = m_queue.presentKHR(&pi);
		lock.unlock();

		m_frame_index.increment();
		m_swapchain.image_index.reset();

		switch (present_result) {
		case vk::Result::eSuccess:
		case vk::Result::eSuboptimalKHR: break;
		case vk::Result::eErrorOutOfDateKHR: recreate_swapchain(); return;
		default: throw Error{"Failed to present Vulkan Swapchain Image"};
		}
	}

	void select_physical_device(CreateInfo const& create_info) {
		auto const gpus = Gpu::get_viable(*m_instance, *m_surface);
		if (gpus.empty()) { throw Error{"No viable GPU found"}; }
		auto physical_devices = std::vector<vk::PhysicalDevice>{};
		physical_devices.reserve(gpus.size());
		for (auto const& gpu : gpus) { physical_devices.push_back(gpu.device); }
		m_physical_device = GpuSelector{create_info.gpu_selector}.select_device(physical_devices);
		m_properties = m_physical_device.getProperties();
		m_depth_format = find_best_depth_format(m_physical_device);
		for (auto const& gpu : gpus) {
			if (gpu.device == m_physical_device) {
				m_queue_family = gpu.queue_family;
				return;
			}
		}

		std::unreachable();
	}

	void create_device() {
		static constexpr auto queue_priority_v = 1.0f;

		auto dci = vk::DeviceCreateInfo{};

		auto dqci = vk::DeviceQueueCreateInfo{};
		dqci.queueCount = 1;
		dqci.queueFamilyIndex = m_queue_family;
		dqci.pQueuePriorities = &queue_priority_v;
		dci.pQueueCreateInfos = &dqci;
		dci.queueCreateInfoCount = 1;

		auto enabled_features = vk::PhysicalDeviceFeatures{};
		auto available_features = m_physical_device.getFeatures();
		enabled_features.fillModeNonSolid = available_features.fillModeNonSolid;
		enabled_features.wideLines = available_features.wideLines;
		enabled_features.samplerAnisotropy = available_features.samplerAnisotropy;
		enabled_features.sampleRateShading = available_features.sampleRateShading;
		dci.pEnabledFeatures = &enabled_features;

		dci.ppEnabledExtensionNames = device_extensions_v.data();
		dci.enabledExtensionCount = static_cast<std::uint32_t>(device_extensions_v.size());

		auto sync_feature = vk::PhysicalDeviceSynchronization2Features{vk::True};
		dci.pNext = &sync_feature;

		auto dr_feature = vk::PhysicalDeviceDynamicRenderingFeatures{vk::True};
		sync_feature.pNext = &dr_feature;

		m_device = m_physical_device.createDeviceUnique(dci);
		if (!m_device) { throw Error{"Failed to create Vulkan Device"}; }
		VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);

		m_queue = m_device->getQueue(m_queue_family, 0);
	}

	void create_swapchain() {
		m_swapchain.present_modes = m_physical_device.getSurfacePresentModesKHR(*m_surface);
		auto const surface_format = Swapchain::get_surface_format(m_physical_device.getSurfaceFormatsKHR(*m_surface));
		m_swapchain.info.imageFormat = surface_format.format;
		m_swapchain.info.surface = *m_surface;
		m_swapchain.info.presentMode = m_swapchain.get_optimal_present_mode();
		m_swapchain.info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
		m_swapchain.info.queueFamilyIndexCount = 1u;
		m_swapchain.info.pQueueFamilyIndices = &m_queue_family;
		m_swapchain.info.imageColorSpace = surface_format.colorSpace;
		m_swapchain.info.imageArrayLayers = 1u;

		static constexpr auto command_pool_flags_v = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
		m_swapchain.command_pool = m_device->createCommandPoolUnique(vk::CommandPoolCreateInfo{command_pool_flags_v, m_queue_family});
		auto const cbai = vk::CommandBufferAllocateInfo{*m_swapchain.command_pool, vk::CommandBufferLevel::ePrimary, buffering_v};
		if (m_device->allocateCommandBuffers(&cbai, m_swapchain.command_buffers.data()) != vk::Result::eSuccess) {
			throw Error{"Failed to allocate Vulkan Swapchain Command Buffers"};
		}

		recreate_swapchain();
	}

	void create_allocator_and_caches() {
		auto vaci = VmaAllocatorCreateInfo{};
		vaci.instance = *m_instance;
		vaci.physicalDevice = m_physical_device;
		vaci.device = *m_device;
		auto dl = VULKAN_HPP_DEFAULT_DISPATCHER;
		auto vkFunc = VmaVulkanFunctions{};
		vkFunc.vkGetInstanceProcAddr = dl.vkGetInstanceProcAddr;
		vkFunc.vkGetDeviceProcAddr = dl.vkGetDeviceProcAddr;
		vaci.pVulkanFunctions = &vkFunc;
		if (vmaCreateAllocator(&vaci, &m_allocator) != VK_SUCCESS) { throw Error{"Failed to create Vulkan Allocator"}; }

		m_descriptor_allocator = std::make_unique<DescriptorAllocator>(this);
		m_pipeline_cache = std::make_unique<PipelineCache>(m_descriptor_allocator.get(), m_depth_format);
		m_sampler_cache = std::make_unique<SamplerCache>(*m_device, m_properties);
		m_texture_cache = std::make_unique<TextureCache>(this);
		m_storage_buffer_pool = std::make_unique<StorageBufferPool>(this);
	}

	void create_dear_imgui() {
		auto const dici = DearImGui::CreateInfo{
			.window = m_window.get(),
			.queue = m_queue,
			.samples = vk::SampleCountFlagBits::e1,
			// .colour = vk::Format::eR8G8B8A8Srgb,
			.colour = m_swapchain.info.imageFormat,
		};
		m_dear_imgui = std::make_unique<DearImGui>(*this, dici);
	}

	auto recreate_swapchain(std::optional<vk::PresentModeKHR> desired_present_mode = {}) -> bool {
		auto const framebuffer = to_vk_extent(detail::get_framebuffer_size(m_window));
		if (framebuffer.width <= 0 || framebuffer.height <= 0) { return false; }

		auto const surface_capabilities = m_physical_device.getSurfaceCapabilitiesKHR(*m_surface);
		m_swapchain.info.compositeAlpha = Swapchain::get_composite_alpha(surface_capabilities);
		m_swapchain.info.imageExtent = Swapchain::get_image_extent(surface_capabilities, framebuffer);
		if (desired_present_mode) { m_swapchain.info.presentMode = *desired_present_mode; }
		m_swapchain.info.minImageCount = Swapchain::get_image_count(surface_capabilities);
		m_swapchain.info.oldSwapchain = *m_swapchain.active;

		m_device->waitIdle();
		m_swapchain.active = get_device().createSwapchainKHRUnique(m_swapchain.info);
		if (!m_swapchain.active) { throw Error{"Failed to create Vulkan Swapchain"}; }

		m_swapchain.images = m_device->getSwapchainImagesKHR(*m_swapchain.active);
		m_defer_queue->push_object(std::move(m_swapchain.views));
		m_swapchain.views.clear();
		m_swapchain.views.reserve(m_swapchain.images.size());
		auto make_image_view = MakeImageView{
			.image = vk::Image{},
			.format = m_swapchain.info.imageFormat,
			.subresource = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
			.type = vk::ImageViewType::e2D,
		};
		for (auto const image : m_swapchain.images) {
			make_image_view.image = image;
			m_swapchain.views.push_back(make_image_view(*m_device));
		}

		for (auto& sync : m_swapchain.syncs) {
			sync.draw = m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
			sync.present = m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
			sync.drawn = m_device->createFenceUnique(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
		}

		m_swapchain.image_index.reset();
		m_swaphain_layout = vk::ImageLayout::eUndefined;

		m_log.info("swapchain extent: [{}x{}] | images: [{}] | vsync: [{}]", m_swapchain.info.imageExtent.width, m_swapchain.info.imageExtent.height,
				   m_swapchain.images.size(), to_vsync_string(m_swapchain.info.presentMode));

		return true;
	}

	void render_imgui(vk::CommandBuffer const command_buffer) {
		auto const rt_colour = RenderTarget::Image{
			.image = m_swapchain.images.at(*m_swapchain.image_index),
			.view = *m_swapchain.views.at(*m_swapchain.image_index),
			.format = m_swapchain.info.imageFormat,
		};
		auto const render_target = RenderTarget{
			.colour = rt_colour,
			.extent = m_swapchain.info.imageExtent,
		};
		auto renderer = DynamicRenderer{this};
		renderer.load_op = DynamicRenderer::LoadOp::eLoad;
		renderer.layouts = {m_swaphain_layout, vk::ImageLayout::ePresentSrcKHR};
		renderer.begin_rendering(render_target, command_buffer);
		auto lock = std::unique_lock{m_queue_mutex};
		m_dear_imgui->render(command_buffer);
		lock.unlock();
		renderer.end_rendering();
		m_swaphain_layout = renderer.layouts.new_layout;
	}

	void blit_to_swapchain(RenderTarget const& render_target, vk::CommandBuffer command_buffer) {
		static constexpr auto isr_v = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		auto const swapchain_image = m_swapchain.images.at(*m_swapchain.image_index);
		m_swaphain_layout = vk::ImageLayout::eUndefined;

		auto barriers = std::array<vk::ImageMemoryBarrier2, 2>{};
		barriers[0].oldLayout = m_swaphain_layout;
		barriers[0].image = swapchain_image;
		barriers[0].subresourceRange = barriers[1].subresourceRange = isr_v;

		if (render_target.is_empty()) {
			if (!render_dear_imgui) {
				barriers[0].newLayout = vk::ImageLayout::ePresentSrcKHR;
				barriers[0].dstStageMask = vk::PipelineStageFlagBits2::eNone;
				barriers[0].dstAccessMask = vk::AccessFlagBits2::eNone;
			}
			m_swaphain_layout = barriers[0].newLayout;
			record_barriers(command_buffer, {barriers.data(), 1});
			return;
		}

		auto const& source_image = render_target.resolve.image ? render_target.resolve : render_target.colour;

		barriers[1].image = source_image.image;
		barriers[0].oldLayout = vk::ImageLayout::eUndefined;
		barriers[1].oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barriers[0].newLayout = vk::ImageLayout::eTransferDstOptimal;
		barriers[1].newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barriers[0].srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
		barriers[0].srcAccessMask = vk::AccessFlagBits2::eNone;
		barriers[0].dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
		barriers[0].dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
		barriers[1].srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		barriers[1].srcAccessMask = vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
		barriers[1].dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
		barriers[1].dstAccessMask = vk::AccessFlagBits2::eTransferRead;

		record_barriers(command_buffer, barriers);

		glm::ivec2 const src_offset = to_glm_vec2(render_target.extent);
		glm::ivec2 const dst_offset = to_glm_vec2(m_swapchain.info.imageExtent);

		auto const ib = vk::ImageBlit2{
			vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
			{vk::Offset3D{}, vk::Offset3D{src_offset.x, src_offset.y, 1}},
			vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
			{vk::Offset3D{}, vk::Offset3D{dst_offset.x, dst_offset.y, 1}},
		};
		auto const bii = vk::BlitImageInfo2{
			barriers[1].image, barriers[1].newLayout, barriers[0].image, barriers[0].newLayout, 1, &ib, vk::Filter::eLinear,
		};
		command_buffer.blitImage2(bii);

		barriers[0].oldLayout = barriers[0].newLayout;
		barriers[0].newLayout = vk::ImageLayout::ePresentSrcKHR;
		barriers[1].oldLayout = barriers[1].newLayout;
		barriers[1].newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barriers[0].srcStageMask = barriers[1].srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
		barriers[0].srcAccessMask = barriers[1].srcAccessMask = vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eTransferRead;
		barriers[0].dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
		barriers[0].dstAccessMask = vk::AccessFlagBits2::eNone;
		barriers[1].dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
		barriers[1].dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead;
		if (render_dear_imgui) {
			// TODO: is this needed? imgui RenderPass will also transition to attachment optimal.
			barriers[0].newLayout = vk::ImageLayout::eColorAttachmentOptimal;
			barriers[0].dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
			barriers[0].dstAccessMask = vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
		}
		m_swaphain_layout = barriers[0].newLayout;
		record_barriers(command_buffer, barriers);
	}

	Logger m_log{"RenderDevice"};
	std::mutex m_queue_mutex{};

	NotNull<GLFWwindow*> m_window;
	vk::UniqueInstance m_instance{};
	vk::UniqueSurfaceKHR m_surface{};
	vk::PhysicalDevice m_physical_device{};
	vk::PhysicalDeviceProperties m_properties{};
	std::uint32_t m_queue_family{};
	vk::Queue m_queue{};
	vk::UniqueDevice m_device{};
	vk::Format m_depth_format{};

	Swapchain m_swapchain{};
	VmaAllocator m_allocator{};
	std::unique_ptr<DeferQueue> m_defer_queue{};
	std::unique_ptr<PipelineCache> m_pipeline_cache{};
	std::unique_ptr<SamplerCache> m_sampler_cache{};
	std::unique_ptr<TextureCache> m_texture_cache{};
	std::unique_ptr<StorageBufferPool> m_storage_buffer_pool{};
	std::unique_ptr<DescriptorAllocator> m_descriptor_allocator{};
	std::unique_ptr<DearImGui> m_dear_imgui{};

	vk::ImageLayout m_swaphain_layout{};

	FrameIndex m_frame_index{};
};
} // namespace
} // namespace levk

auto levk::create_render_device(RenderDeviceCreateInfo const& create_info) -> std::unique_ptr<IRenderDevice> {
	return std::make_unique<RenderDevice>(create_info);
}
