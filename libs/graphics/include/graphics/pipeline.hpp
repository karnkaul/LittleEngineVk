#pragma once
#include <unordered_set>
#include <core/hash.hpp>
#include <core/ref.hpp>
#include <core/view.hpp>
#include <graphics/descriptor_set.hpp>
#include <graphics/types.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class Device;
class Shader;
class CommandBuffer;

class Pipeline final {
  public:
	struct CreateInfo {
		struct Fixed {
			vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics;
			VertexInputInfo vertexInput;
			vk::PipelineRasterizationStateCreateInfo rasterizerState;
			vk::PipelineMultisampleStateCreateInfo multisamplerState;
			vk::PipelineColorBlendAttachmentState colorBlendAttachment;
			vk::PipelineDepthStencilStateCreateInfo depthStencilState;
			std::unordered_set<vk::DynamicState> dynamicStates;
		};
		struct Dynamic {
			CView<Shader> shader;
			vk::RenderPass renderPass;
		};

		Fixed fixedState;
		Dynamic dynamicState;
		u32 subpass = 0;
		u32 rotateCount = 2;
	};
	struct SetIndex {
		u32 set = 0;
		std::size_t index = 0;
	};

	Pipeline(VRAM& vram, CreateInfo createInfo, Hash id);
	Pipeline(Pipeline&&);
	Pipeline& operator=(Pipeline&&);
	~Pipeline();

	bool reconstruct(CreateInfo::Dynamic const& newState = {});
	vk::PipelineLayout layout() const;
	vk::DescriptorSetLayout setLayout(u32 set) const;
	SetFactory& setFactory(u32 set);
	void swapSets();

	template <typename T>
	bool writeBuffer(SetIndex set, u32 binding, T const& data, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);
	template <typename T, typename Cont = Span<T>>
	bool writeBuffers(SetIndex set, u32 binding, Cont&& data, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);
	bool writeTextures(SetIndex set, u32 binding, Span<Texture> textures);

	void bindSets(CommandBuffer& cmd, Span<DescriptorSet*> descriptorSets) const;
	void bindSet(CommandBuffer& cmd, SetIndex set);

	Hash id() const noexcept;

  private:
	bool construct(bool bFixed);
	void destroy(bool bFixed);
	DescriptorSet& descriptorSet(SetIndex bindSet);

	struct Storage {
		struct {
			vk::Pipeline pipeline;
		} dynamic;
		struct {
			vk::PipelineLayout layout;
			std::vector<vk::DescriptorSetLayout> setLayouts;
		} fixed;
		std::vector<SetFactory> setFactories;
		Hash id;
	};
	struct Metadata {
		CreateInfo createInfo;
	};

	Storage m_storage;
	Metadata m_metadata;
	Ref<VRAM> m_vram;
	Ref<Device> m_device;

	friend struct Hasher;
	friend class CommandBuffer;
};

// impl

template <typename T>
bool Pipeline::writeBuffer(SetIndex set, u32 binding, T const& data, vk::DescriptorType type) {
	return descriptorSet(set).writeBuffer(binding, data, type);
}
template <typename T, typename Cont>
bool Pipeline::writeBuffers(SetIndex set, u32 binding, Cont&& data, vk::DescriptorType type) {
	return descriptorSet(set).writeBuffers(binding, std::forward<Cont>(data), type);
}
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
