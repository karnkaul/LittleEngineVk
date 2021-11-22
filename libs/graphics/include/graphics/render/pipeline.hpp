#pragma once
#include <core/hash.hpp>
#include <core/utils/std_hash.hpp>
#include <graphics/render/buffering.hpp>
#include <graphics/render/descriptor_set.hpp>
#include <graphics/render/vertex_input.hpp>
#include <graphics/shader.hpp>
#include <unordered_set>

namespace le::graphics {
class Device;
class Shader;
class CommandBuffer;
class VRAM;

class Pipeline final {
  public:
	struct CreateInfo {
		struct Fixed {
			VertexInputInfo vertexInput;
			vk::PipelineRasterizationStateCreateInfo rasterizerState;
			vk::PipelineMultisampleStateCreateInfo multisamplerState;
			vk::PipelineColorBlendAttachmentState colorBlendAttachment;
			vk::PipelineDepthStencilStateCreateInfo depthStencilState;
			vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
			std::unordered_set<vk::DynamicState> dynamicStates;
		};

		Fixed fixedState;
		vk::RenderPass renderPass;
		vk::PipelineCache cache;
		vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics;
		Buffering buffering = 2_B;
		u32 subpass = 0;
	};
	struct SetIndex {
		u32 set = 0;
		std::size_t index = 0;
	};

	Pipeline(not_null<VRAM*> vram, Shader const& shader, CreateInfo createInfo, Hash id);

	std::optional<vk::Pipeline> constructVariant(Hash id, CreateInfo::Fixed const& fixed);
	std::optional<vk::Pipeline> variant(Hash id) const;

	bool reconstruct(Shader const& shader);
	vk::PipelineBindPoint bindPoint() const;
	vk::PipelineLayout layout() const;
	ShaderInput& shaderInput();
	ShaderInput const& shaderInput() const;

	DescriptorPool makeSetPool(u32 set, Buffering buffering) const;
	std::unordered_map<u32, DescriptorPool> makeSetPools(Buffering buffering) const;

	void bindSet(CommandBuffer cb, u32 set, std::size_t idx) const;
	void bindSet(CommandBuffer cb, std::initializer_list<u32> sets, std::size_t idx) const;
	void swap() { m_storage.input.swap(); }

	Hash id() const noexcept;
	CreateInfo::Fixed fixedState(Hash variant = {}) const noexcept;

	not_null<VRAM*> m_vram;
	not_null<Device*> m_device;

  private:
	bool setup(Shader const& shader);
	bool construct(CreateInfo& out_info, vk::Pipeline& out_pipe);

	struct Storage {
		ShaderInput input;
		struct {
			std::unordered_map<Hash, Deferred<vk::Pipeline>> variants;
			Shader::ModuleMap modules;
			Deferred<vk::Pipeline> main;
		} dynamic;
		struct {
			Deferred<vk::PipelineLayout> layout;
			std::vector<Deferred<vk::DescriptorSetLayout>> setLayouts;
			std::vector<ktl::fixed_vector<BindingInfo, 16>> bindingInfos;
		} fixed;
		vk::PipelineCache cache;
		Hash id;
	};
	struct Metadata {
		std::string_view name;
		CreateInfo main;
		std::unordered_map<Hash, CreateInfo::Fixed> variants;
	};

	Storage m_storage;
	Metadata m_metadata;

	friend struct Hasher;
};

// impl

inline Hash Pipeline::id() const noexcept { return m_storage.id; }
} // namespace le::graphics

namespace std {
template <>
struct hash<le::graphics::Pipeline> {
	size_t operator()(le::graphics::Pipeline const& pipe) const { return pipe.id(); }
};
} // namespace std
