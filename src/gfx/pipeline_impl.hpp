#pragma once
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/resources/resource_types.hpp>
#include <gfx/common.hpp>
#if defined(LEVK_RESOURCES_HOT_RELOAD)
#include <core/delegate.hpp>
#endif

namespace le::gfx
{
class PipelineImpl final
{
public:
	struct Info final
	{
		using CCFlag = vk::ColorComponentFlagBits;
		std::string shaderID;
		std::vector<vk::VertexInputBindingDescription> vertexBindings;
		std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
		std::vector<vk::PushConstantRange> pushConstantRanges;
		std::set<vk::DynamicState> dynamicStates;
		vk::RenderPass renderPass;
		vk::PolygonMode polygonMode;
		vk::CullModeFlagBits cullMode;
		vk::FrontFace frontFace;
		vk::ColorComponentFlags colourWriteMask = CCFlag::eR | CCFlag::eG | CCFlag::eB | CCFlag::eA;
		f32 staticLineWidth = 1.0f;
		res::Shader shader;
		Pipeline::Flags flags;
	};

public:
	vk::Pipeline m_pipeline;
	vk::PipelineLayout m_layout;

private:
	Info m_info;
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	Delegate<>::Token m_reloadToken;
	bool m_bShaderReloaded = false;
#endif

public:
	PipelineImpl();
	PipelineImpl(PipelineImpl&&);
	PipelineImpl& operator=(PipelineImpl&&);
	~PipelineImpl();

public:
	bool create(Info info);
	bool update(RenderPass const& renderPass);
	void destroy();

private:
	bool create();
};

namespace pipes
{
PipelineImpl& find(Pipeline const& pipe, RenderPass const& renderPass);
void deinit();
} // namespace pipes
} // namespace le::gfx
