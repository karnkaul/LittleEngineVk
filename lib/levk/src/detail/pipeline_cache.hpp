#include <levk/core/enum_array.hpp>
#include <levk/core/error.hpp>
#include <levk/core/hash_combine.hpp>
#include <levk/logger.hpp>
#include <levk/render_device.hpp>
#include <spirv_glsl.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <map>

namespace levk {
inline constexpr auto shader_stage_flags_v = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

class DescriptorAllocator {
  public:
	explicit DescriptorAllocator(NotNull<IRenderDevice*> device) : m_device(device) {}

	[[nodiscard]] auto get_device() const -> IRenderDevice& { return *m_device; }

	[[nodiscard]] auto allocate(vk::DescriptorSetLayout const layout) -> vk::DescriptorSet {
		if (auto const it = m_allocators.find(layout); it != m_allocators.end()) { return it->second.allocate(); }
		auto const [it, _] = m_allocators.insert_or_assign(layout, Allocator{m_device, layout});
		return it->second.allocate();
	}

	void next_frame() {
		for (auto& [_, allocator] : m_allocators) { allocator.next_frame(); }
	}

  private:
	class Allocator {
	  public:
		explicit Allocator(NotNull<IRenderDevice*> device, vk::DescriptorSetLayout const layout) : m_device(device), m_layout(layout) {}

		void next_frame() { m_frames.at(m_device->get_frame_index()).next = 0; }

		[[nodiscard]] auto allocate() -> vk::DescriptorSet {
			auto& frame = m_frames.at(m_device->get_frame_index());

			static constexpr std::size_t bloat_size_v{10000};
			if (frame.sets.size() > bloat_size_v) { m_log.warn("'{}' descriptors already allocated, possible leak?", frame.sets.size()); }

			if (frame.next < frame.sets.size()) { return frame.sets.at(frame.next++); }

			if (frame.pools.empty()) { frame.pools.push_back(create_pool()); }
			auto pool = *frame.pools.back();
			auto ret = try_allocate(pool);
			if (!ret) {
				frame.pools.push_back(create_pool());
				pool = *frame.pools.back();
				ret = try_allocate(pool);
			}
			if (!ret) { throw Error{"Failed to allocate Vulkan Descriptor Set"}; }

			++frame.next;
			frame.sets.push_back(ret);
			return frame.sets.back();
		}

	  private:
		static constexpr auto max_sets_v = std::uint32_t{64};
		static constexpr auto descriptor_count_v = std::uint32_t{64};

		[[nodiscard]] auto try_allocate(vk::DescriptorPool const pool) const -> vk::DescriptorSet {
			auto const dsai = vk::DescriptorSetAllocateInfo{
				pool,
				1,
				&m_layout,
			};
			auto ret = vk::DescriptorSet{};
			if (m_device->get_device().allocateDescriptorSets(&dsai, &ret) != vk::Result::eSuccess) { return {}; }
			return ret;
		}

		[[nodiscard]] auto create_pool() const -> vk::UniqueDescriptorPool {
			static constexpr auto pool_sizes = std::array{
				vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, descriptor_count_v},
				vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, descriptor_count_v},
				vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, descriptor_count_v},
			};
			static constexpr auto dpci = vk::DescriptorPoolCreateInfo{
				vk::DescriptorPoolCreateFlags{},
				max_sets_v,
				static_cast<std::uint32_t>(pool_sizes.size()),
				pool_sizes.data(),
			};
			return m_device->get_device().createDescriptorPoolUnique(dpci);
		}

		struct Frame {
			std::vector<vk::UniqueDescriptorPool> pools{};
			std::vector<vk::DescriptorSet> sets{};
			std::size_t next{};
		};

		Logger m_log{"DescriptorAllocator"};
		NotNull<IRenderDevice*> m_device;
		vk::DescriptorSetLayout m_layout{};

		Buffered<Frame> m_frames{};
	};

	NotNull<IRenderDevice*> m_device;

	std::unordered_map<vk::DescriptorSetLayout, Allocator> m_allocators{};
};

class PipelineCache {
  public:
	using State = PipelineState;
	using Shader = RenderShader;

	explicit PipelineCache(NotNull<DescriptorAllocator*> descriptor_allocator, vk::Format const depth_format)
		: m_descriptor_allocator(descriptor_allocator), m_device(descriptor_allocator->get_device().get_device()), m_depth_format(depth_format) {
		for (auto i = VertexBinding{}; i < VertexBinding::eCOUNT_; i = VertexBinding(int(i) + 1)) { m_vertex_input[i] = VertexInput::create_for(i); }
	}

	[[nodiscard]] auto get_or_create(Shader const vertex, Shader const fragment, State const& state) -> Ptr<IPipeline> {
		auto const shader_key = ShaderKey{vertex, fragment};
		auto const state_key = StateKey{state};
		auto it_map = m_pipeline_maps.find(shader_key);
		if (it_map == m_pipeline_maps.end()) {
			auto map = PipelineMap{};
			auto vs = build_shader(vertex);
			auto fs = build_shader(fragment);
			if (!vs.shader || !fs.shader) { return {}; }
			map.layout = build_layout(std::move(vs), std::move(fs));
			assert(map.layout.layout);
			auto [it, _] = m_pipeline_maps.insert_or_assign(shader_key, std::move(map));
			it_map = it;
		}
		auto& map = it_map->second;
		auto it_pipe = map.pipelines.find(state_key);
		if (it_pipe == map.pipelines.end()) {
			auto pipeline = build_pipeline(map.layout, state);
			if (!pipeline) { return {}; }
			auto pipeline_impl = PipelineImpl{m_descriptor_allocator, std::move(pipeline), map.layout};
			auto [it, _] = map.pipelines.insert_or_assign(state_key, std::move(pipeline_impl));
			it_pipe = it;
		}
		return &it_pipe->second;
	}

  private:
	using SetBindings = std::vector<vk::DescriptorSetLayoutBinding>;

	struct ShaderKey {
		explicit ShaderKey(Shader const& vertex, Shader const& fragment) : vertex(vertex), fragment(fragment), cached_hash(compute_hash(vertex, fragment)) {}

		static auto compute_hash(Shader const v, Shader const f) -> std::size_t {
			auto ret = std::size_t{};
			auto const combine_shader = [&ret](std::span<std::byte const> bytes) {
				for (auto const byte : bytes) { hash_combine(ret, byte); }
			};
			combine_shader(v);
			combine_shader(f);
			return ret;
		}

		auto operator==(ShaderKey const& rhs) const -> bool { return cached_hash == rhs.cached_hash; }

		Shader vertex{};
		Shader fragment{};
		std::size_t cached_hash{};
	};

	struct StateKey {
		explicit StateKey(State const& state) : state(state), cached_hash(compute_hash(state)) {}

		static auto compute_hash(State const& state) -> std::size_t {
			return make_combined_hash(state.primitive_state.topology, state.primitive_state.vertex_binding, state.primitive_state.alpha_blend,
									  state.pass_state.polygon_mode, state.pass_state.colour_format, state.pass_state.depth_format, state.pass_state.samples,
									  state.pass_state.depth_compare, state.pass_state.depth_test);
		}

		[[nodiscard]] auto hash() const -> std::size_t { return cached_hash; }

		auto operator==(StateKey const& rhs) const -> bool { return state == rhs.state; }

		State state{};
		std::size_t cached_hash{};
	};

	struct KeyHasher {
		[[nodiscard]] auto operator()(ShaderKey const& key) const -> std::size_t { return key.cached_hash; }
		[[nodiscard]] auto operator()(StateKey const& key) const -> std::size_t { return key.cached_hash; }
	};

	struct VertexInput {
		std::vector<vk::VertexInputAttributeDescription> attributes{};
		std::vector<vk::VertexInputBindingDescription> bindings{};

		static auto create_for(VertexBinding binding) -> VertexInput {
			auto ret = VertexInput{};

			ret.bindings = {
				// vertices
				vk::VertexInputBindingDescription{0, sizeof(Vertex)},
			};
			ret.attributes = {
				vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
				vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)},
				vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, rgba)},
				vk::VertexInputAttributeDescription{3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
				vk::VertexInputAttributeDescription{4, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent)},
			};

			if (binding == VertexBinding::eSkinned) {
				// joints
				// TODO: use constant here and in vertex layout.
				ret.bindings.push_back(vk::VertexInputBindingDescription{5, sizeof(glm::uvec4)});
				ret.attributes.emplace_back(5, 5, vk::Format::eR32G32B32A32Uint);

				// weights
				ret.bindings.push_back(vk::VertexInputBindingDescription{6, sizeof(glm::vec4)});
				ret.attributes.emplace_back(6, 6, vk::Format::eR32G32B32A32Sfloat);
			}

			return ret;
		}
	};

	struct SpirV {
		std::vector<std::uint32_t> code{};
		vk::UniqueShaderModule shader{};
	};

	struct Layout {
		struct {
			vk::UniqueShaderModule vertex{};
			vk::UniqueShaderModule fragment{};
		} shader{};
		vk::UniquePipelineLayout layout{};
		std::vector<vk::UniqueDescriptorSetLayout> unique_set_layouts{};
		std::vector<vk::DescriptorSetLayout> set_layouts{};
		std::vector<SetBindings> set_layout{};
	};

	class PipelineImpl : public IPipeline {
	  public:
		explicit PipelineImpl(NotNull<DescriptorAllocator*> descriptor_allocator, vk::UniquePipeline pipeline, Layout const& layout)
			: m_descriptor_allocator(descriptor_allocator), m_pipeline(std::move(pipeline)), m_layout(*layout.layout), m_set_layouts(layout.set_layouts),
			  m_set_layout(layout.set_layout) {}

	  private:
		[[nodiscard]] auto get_pipeline() const -> vk::Pipeline final { return *m_pipeline; }
		[[nodiscard]] auto get_layout() const -> vk::PipelineLayout final { return m_layout; }

		[[nodiscard]] auto get_bindings(std::uint32_t const set) const -> std::span<vk::DescriptorSetLayoutBinding const> final {
			if (set >= m_set_layout.size()) { return {}; }
			return m_set_layout[set];
		}

		auto allocate_descriptor_set(std::uint32_t set_number) -> vk::DescriptorSet final {
			if (set_number >= m_set_layouts.size()) { return {}; }
			return m_descriptor_allocator->allocate(m_set_layouts[set_number]);
		}

		void push_constants(vk::CommandBuffer command_buffer, void const* data, std::size_t size) const final {
			command_buffer.pushConstants(m_layout, shader_stage_flags_v, 0, static_cast<std::uint32_t>(size), data);
		}

		void update_descriptor_sets(std::span<vk::WriteDescriptorSet const> writes) final {
			m_descriptor_allocator->get_device().get_device().updateDescriptorSets(writes, {});
		}

		void bind_descriptor_sets(vk::CommandBuffer command_buffer, std::span<vk::DescriptorSet const> descriptor_sets,
								  std::uint32_t const first_set) const final {
			command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_layout, first_set, descriptor_sets, {});
		}

		NotNull<DescriptorAllocator*> m_descriptor_allocator;
		vk::UniquePipeline m_pipeline{};
		vk::PipelineLayout m_layout{};
		std::span<vk::DescriptorSetLayout const> m_set_layouts{};
		std::span<SetBindings const> m_set_layout{};
	};

	struct PipelineMap {
		Layout layout{};
		std::unordered_map<StateKey, PipelineImpl, KeyHasher> pipelines{};
	};

	struct SpirVLayout {
		struct Shader {
			std::span<std::uint32_t const> vertex{};
			std::span<std::uint32_t const> fragment{};
		};

		std::vector<SetBindings> set_layout{};
		std::optional<vk::PushConstantRange> push_constant{};
	};

	[[nodiscard]] static auto build_spirv_layout(SpirVLayout::Shader const& shader) -> SpirVLayout {
		auto ret = SpirVLayout{};
		std::map<std::uint32_t, std::map<std::uint32_t, vk::DescriptorSetLayoutBinding>> set_layout_bindings{};
		auto populate = [&set_layout_bindings, &ret](std::span<std::uint32_t const> code) {
			auto compiler = spirv_cross::CompilerGLSL{code.data(), code.size()};
			auto resources = compiler.get_shader_resources();
			auto set_resources = [&compiler, &set_layout_bindings](std::span<spirv_cross::Resource const> resources, vk::DescriptorType const type) {
				for (auto const& resource : resources) {
					auto const set_number = compiler.get_decoration(resource.id, spv::Decoration::DecorationDescriptorSet);
					auto& layout = set_layout_bindings[set_number];
					auto const binding_number = compiler.get_decoration(resource.id, spv::Decoration::DecorationBinding);
					auto& dslb = layout[binding_number];
					dslb.binding = binding_number;
					dslb.descriptorType = type;
					dslb.stageFlags = shader_stage_flags_v;
					auto const& type = compiler.get_type(resource.type_id);
					if (type.array.size() == 0) {
						dslb.descriptorCount = std::max(dslb.descriptorCount, 1u);
					} else {
						dslb.descriptorCount = type.array[0];
					}
				}
			};
			set_resources(resources.uniform_buffers, vk::DescriptorType::eUniformBuffer);
			set_resources(resources.storage_buffers, vk::DescriptorType::eStorageBuffer);
			set_resources(resources.sampled_images, vk::DescriptorType::eCombinedImageSampler);

			if (!resources.push_constant_buffers.empty()) {
				auto const& resource = resources.push_constant_buffers.front();
				auto const& type = compiler.get_type(resource.base_type_id);
				auto const size = static_cast<std::uint32_t>(compiler.get_declared_struct_size(type));
				auto const func = [&compiler, size](auto const& r) {
					auto const& type = compiler.get_type(r.base_type_id);
					return size == static_cast<std::uint32_t>(compiler.get_declared_struct_size(type));
				};
				if (!std::ranges::all_of(resources.push_constant_buffers, func)) {
					throw Error{"Push constants ranges need to be identical in a shader program"};
				}
				ret.push_constant = vk::PushConstantRange{shader_stage_flags_v, 0, size};
			}
		};
		populate(shader.vertex);
		populate(shader.fragment);

		for (auto& [_, bindings] : set_layout_bindings) {
			for (auto& [_, binding] : bindings) { binding.stageFlags |= shader_stage_flags_v; }
		}

		auto next_set = std::uint32_t{};
		for (auto& [set, layouts] : set_layout_bindings) {
			while (next_set < set) {
				ret.set_layout.emplace_back(); // fill set hole
				++next_set;
			}
			next_set = set + 1;

			auto& set_bindings = ret.set_layout.emplace_back();
			auto next_binding = std::uint32_t{};
			for (auto const& [binding, dslb] : layouts) {
				while (next_binding < binding) {
					auto dslb = vk::DescriptorSetLayoutBinding{};
					dslb.binding = next_binding;
					set_bindings.push_back(dslb); // fill binding hole
					++next_binding;
				}
				next_binding = binding + 1;
				set_bindings.push_back(dslb);
			}
		}
		return ret;
	}

	[[nodiscard]] auto build_shader(std::span<std::byte const> bytes) const -> SpirV {
		assert(bytes.size() % sizeof(std::uint32_t) == 0);
		auto ret = SpirV{};
		ret.code.resize(bytes.size() / sizeof(std::uint32_t));
		std::memcpy(ret.code.data(), bytes.data(), bytes.size());
		auto const smci = vk::ShaderModuleCreateInfo{vk::ShaderModuleCreateFlags{}, std::span{ret.code}.size_bytes(), ret.code.data()};
		ret.shader = m_device.createShaderModuleUnique(smci);
		return ret;
	}

	[[nodiscard]] auto build_layout(SpirV vertex, SpirV fragment) const -> Layout {
		auto ret = Layout{};
		ret.shader.vertex = std::move(vertex.shader);
		ret.shader.fragment = std::move(fragment.shader);
		auto spirv_layout = build_spirv_layout({vertex.code, fragment.code});
		ret.unique_set_layouts.reserve(spirv_layout.set_layout.size());
		ret.set_layouts.reserve(spirv_layout.set_layout.size());
		for (auto const& set_bindings : spirv_layout.set_layout) {
			auto const dslci = vk::DescriptorSetLayoutCreateInfo{
				vk::DescriptorSetLayoutCreateFlags{},
				static_cast<std::uint32_t>(set_bindings.size()),
				set_bindings.data(),
			};
			ret.unique_set_layouts.push_back(m_device.createDescriptorSetLayoutUnique(dslci));
			ret.set_layouts.push_back(*ret.unique_set_layouts.back());
		}
		auto plci = vk::PipelineLayoutCreateInfo{};
		plci.setLayoutCount = static_cast<std::uint32_t>(spirv_layout.set_layout.size());
		plci.pSetLayouts = ret.set_layouts.data();
		if (spirv_layout.push_constant) {
			plci.pushConstantRangeCount = 1;
			plci.pPushConstantRanges = &*spirv_layout.push_constant;
		}
		ret.layout = m_device.createPipelineLayoutUnique(plci);
		ret.set_layout = std::move(spirv_layout.set_layout);
		return ret;
	}

	[[nodiscard]] auto build_pipeline(Layout const& layout, State const& state) const -> vk::UniquePipeline {
		auto shader_stages = std::array<vk::PipelineShaderStageCreateInfo, 2>{};
		shader_stages[0].stage = vk::ShaderStageFlagBits::eVertex;
		shader_stages[1].stage = vk::ShaderStageFlagBits::eFragment;
		shader_stages[0].pName = shader_stages[1].pName = "main";

		shader_stages[0].module = *layout.shader.vertex;
		shader_stages[1].module = *layout.shader.fragment;

		auto pvisci = vk::PipelineVertexInputStateCreateInfo{};
		auto const& vertex_input = m_vertex_input[state.primitive_state.vertex_binding];
		pvisci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertex_input.attributes.size());
		pvisci.pVertexAttributeDescriptions = vertex_input.attributes.data();
		pvisci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertex_input.bindings.size());
		pvisci.pVertexBindingDescriptions = vertex_input.bindings.data();

		auto gpci = vk::GraphicsPipelineCreateInfo{};
		gpci.pVertexInputState = &pvisci;
		gpci.stageCount = static_cast<std::uint32_t>(shader_stages.size());
		gpci.pStages = shader_stages.data();

		auto prsci = vk::PipelineRasterizationStateCreateInfo{};
		prsci.polygonMode = state.pass_state.polygon_mode;
		prsci.cullMode = vk::CullModeFlagBits::eNone;
		gpci.pRasterizationState = &prsci;

		auto pdssci = vk::PipelineDepthStencilStateCreateInfo{};
		pdssci.depthTestEnable = pdssci.depthWriteEnable = state.pass_state.depth_test ? vk::True : vk::False;
		pdssci.depthCompareOp = state.pass_state.depth_compare;
		gpci.pDepthStencilState = &pdssci;

		auto const piasci = vk::PipelineInputAssemblyStateCreateInfo{{}, state.primitive_state.topology};
		gpci.pInputAssemblyState = &piasci;

		auto pcbas = vk::PipelineColorBlendAttachmentState{};
		using CCF = vk::ColorComponentFlagBits;
		pcbas.colorWriteMask = CCF::eR | CCF::eG | CCF::eB | CCF::eA;
		pcbas.blendEnable = state.primitive_state.alpha_blend ? vk::True : vk::False;
		pcbas.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		pcbas.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		pcbas.colorBlendOp = vk::BlendOp::eAdd;
		pcbas.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		pcbas.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		pcbas.alphaBlendOp = vk::BlendOp::eAdd;
		auto pcbsci = vk::PipelineColorBlendStateCreateInfo();
		pcbsci.attachmentCount = 1;
		pcbsci.pAttachments = &pcbas;
		gpci.pColorBlendState = &pcbsci;

		auto pdsci = vk::PipelineDynamicStateCreateInfo();
		auto const pdscis = std::array{
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
			vk::DynamicState::eLineWidth,
		};
		pdsci = vk::PipelineDynamicStateCreateInfo({}, static_cast<std::uint32_t>(pdscis.size()), pdscis.data());
		gpci.pDynamicState = &pdsci;

		auto pvsci = vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);
		gpci.pViewportState = &pvsci;

		auto pmsci = vk::PipelineMultisampleStateCreateInfo{};
		pmsci.rasterizationSamples = state.pass_state.samples;
		pmsci.sampleShadingEnable = vk::False;
		gpci.pMultisampleState = &pmsci;

		gpci.layout = *layout.layout;

		auto prci = vk::PipelineRenderingCreateInfo{};
		prci.colorAttachmentCount = state.pass_state.colour_format == vk::Format::eUndefined ? 0 : 1;
		prci.pColorAttachmentFormats = &state.pass_state.colour_format;
		prci.depthAttachmentFormat = state.pass_state.depth_format.value_or(m_depth_format);

		gpci.pNext = &prci;

		auto ret = vk::Pipeline{};
		if (m_device.createGraphicsPipelines({}, 1, &gpci, {}, &ret) != vk::Result::eSuccess) { return {}; }

		return vk::UniquePipeline{ret, m_device};
	}

	NotNull<DescriptorAllocator*> m_descriptor_allocator;
	vk::Device m_device{};
	vk::Format m_depth_format{};

	EnumArray<VertexBinding, VertexInput> m_vertex_input{};

	std::unordered_map<ShaderKey, PipelineMap, KeyHasher> m_pipeline_maps{};
};
} // namespace levk
