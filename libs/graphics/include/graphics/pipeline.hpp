#pragma once
#include <unordered_set>
#include <core/hash.hpp>
#include <core/utils/std_hash.hpp>
#include <graphics/descriptor_set.hpp>
#include <graphics/render/buffering.hpp>
#include <kt/result/result.hpp>

namespace le::graphics {
class Device;
class Shader;
class CommandBuffer;
class VRAM;

struct VertexInputInfo {
	std::vector<vk::VertexInputBindingDescription> bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;
};

template <typename T>
struct VertexInfoFactory {
	struct VertexInputInfo operator()(u32 binding) const;
};

class Pipeline final {
  public:
	struct CreateInfo {
		struct Fixed {
			VertexInputInfo vertexInput;
			vk::PipelineRasterizationStateCreateInfo rasterizerState;
			vk::PipelineMultisampleStateCreateInfo multisamplerState;
			vk::PipelineColorBlendAttachmentState colorBlendAttachment;
			vk::PipelineDepthStencilStateCreateInfo depthStencilState;
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

	kt::result<vk::Pipeline, void> constructVariant(Hash id, Shader const& shader, CreateInfo::Fixed fixed);
	kt::result<vk::Pipeline, void> variant(Hash id) const;

	bool reconstruct(Shader const& shader);
	vk::PipelineBindPoint bindPoint() const;
	vk::PipelineLayout layout() const;
	vk::DescriptorSetLayout setLayout(u32 set) const;
	ShaderInput& shaderInput();
	ShaderInput const& shaderInput() const;

	SetPool makeSetPool(u32 set, Buffering buffering) const;
	std::unordered_map<u32, SetPool> makeSetPools(Buffering buffering) const;

	void bindSet(CommandBuffer cb, u32 set, std::size_t idx) const;
	void bindSet(CommandBuffer cb, std::initializer_list<u32> sets, std::size_t idx) const;

	Hash id() const noexcept;

  private:
	bool construct(Shader const& shader, CreateInfo& out_info, vk::Pipeline& out_pipe, bool bFixed);

	struct Storage {
		ShaderInput input;
		struct {
			Deferred<vk::Pipeline> main;
			std::unordered_map<Hash, Deferred<vk::Pipeline>> variants;
		} dynamic;
		struct {
			Deferred<vk::PipelineLayout> layout;
			std::vector<Deferred<vk::DescriptorSetLayout>> setLayouts;
			std::vector<kt::fixed_vector<BindingInfo, 16>> bindingInfos;
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
	not_null<VRAM*> m_vram;
	not_null<Device*> m_device;

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
