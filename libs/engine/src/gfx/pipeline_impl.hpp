#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "engine/window/common.hpp"
#include "engine/gfx/pipeline.hpp"

namespace le::gfx
{
class PipelineImpl final
{
public:
	struct Info final
	{
		using CCFlag = vk::ColorComponentFlagBits;
		std::string name;
		std::string shaderID;
		std::vector<vk::VertexInputBindingDescription> vertexBindings;
		std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
		std::vector<vk::PushConstantRange> pushConstantRanges;
		std::set<vk::DynamicState> dynamicStates;
		vk::RenderPass renderPass;
		vk::PolygonMode polygonMode;
		vk::CullModeFlagBits cullMode;
		vk::FrontFace frontFace;
		vk::DescriptorSetLayout samplerLayout;
		vk::ColorComponentFlags colourWriteMask = CCFlag::eR | CCFlag::eG | CCFlag::eB | CCFlag::eA;
		WindowID window;
		f32 staticLineWidth = 1.0f;
		class Shader* pShader = nullptr;
		Pipeline::Flags flags;
	};

public:
	static std::string const s_tName;
	std::string m_name;

public:
	vk::Pipeline m_pipeline;
	vk::PipelineLayout m_layout;

private:
	Info m_info;
#if defined(LEVK_ASSET_HOT_RELOAD)
	bool m_bShaderReloaded = false;
#endif

public:
	PipelineImpl();
	PipelineImpl(PipelineImpl&&);
	PipelineImpl& operator=(PipelineImpl&&);
	~PipelineImpl();

public:
	bool create(Info info);
	bool update(vk::DescriptorSetLayout samplerLayout);
	void destroy();

#if defined(LEVK_ASSET_HOT_RELOAD)
	void pollShaders();
#endif

private:
	bool create();
};
} // namespace le::gfx
