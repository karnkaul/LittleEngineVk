#pragma once
#include <unordered_set>
#include <core/hash.hpp>
#include <core/ref.hpp>
#include <core/utils/std_hash.hpp>
#include <graphics/descriptor_set.hpp>
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
		vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics;
		u32 subpass = 0;
		u32 rotateCount = 2;
	};
	struct SetIndex {
		u32 set = 0;
		std::size_t index = 0;
	};

	Pipeline(VRAM& vram, Shader const& shader, CreateInfo createInfo, Hash id);
	Pipeline(Pipeline&&);
	Pipeline& operator=(Pipeline&&);
	~Pipeline();

	kt::result_t<vk::Pipeline, void> constructVariant(Hash id, Shader const& shader, CreateInfo::Fixed fixed);
	kt::result_t<vk::Pipeline, void> variant(Hash id) const;

	bool reconstruct(Shader const& shader);
	vk::PipelineBindPoint bindPoint() const;
	vk::PipelineLayout layout() const;
	vk::DescriptorSetLayout setLayout(u32 set) const;
	ShaderInput& shaderInput();
	ShaderInput const& shaderInput() const;

	SetPool makeSetPool(u32 set, std::size_t rotateCount = 0) const;
	std::unordered_map<u32, SetPool> makeSetPools(std::size_t rotateCount = 0) const;

	Hash id() const noexcept;

  private:
	bool construct(Shader const& shader, CreateInfo& out_info, vk::Pipeline& out_pipe, bool bFixed);
	void destroy();
	void destroy(vk::Pipeline pipeline);

	struct Storage {
		ShaderInput input;
		struct {
			vk::Pipeline main;
			std::unordered_map<Hash, vk::Pipeline> variants;
		} dynamic;
		struct {
			vk::PipelineLayout layout;
			std::vector<vk::DescriptorSetLayout> setLayouts;
			std::vector<std::vector<BindingInfo>> bindingInfos;
		} fixed;
		Hash id;
	};
	struct Metadata {
		CreateInfo main;
		std::unordered_map<Hash, CreateInfo::Fixed> variants;
	};

	Storage m_storage;
	Metadata m_metadata;
	Ref<VRAM> m_vram;
	Ref<Device> m_device;

	friend struct Hasher;
};

// impl

inline Hash Pipeline::id() const noexcept {
	return m_storage.id;
}
} // namespace le::graphics

namespace std {
template <>
struct hash<le::graphics::Pipeline> {
	size_t operator()(le::graphics::Pipeline const& pipe) const {
		return pipe.id();
	}
};
} // namespace std
