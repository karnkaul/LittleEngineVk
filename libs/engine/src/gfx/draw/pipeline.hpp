#pragma once
#include <string>
#include "core/std_types.hpp"
#include "engine/window/common.hpp"
#include "resources.hpp"

namespace le::gfx
{
class Pipeline final
{
public:
	struct Info final
	{
		vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
		vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eNone;
		vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
		vk::ColorComponentFlags colourWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
												  | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		std::string name;
		std::string shaderID;
		std::set<vk::DynamicState> dynamicStates;
		std::vector<vk::DescriptorSetLayout> setLayouts;
		f32 staticLineWidth = 1.0f;
		class Shader* pShader = nullptr;
		bool bBlend = false;
	};

public:
	static std::string const s_tName;

public:
	vk::Pipeline m_pipeline;
	vk::PipelineLayout m_layout;
	vk::Fence m_drawing;

#if defined(LEVK_RESOURCE_HOT_RELOAD)
private:
	struct
	{
		vk::Pipeline pipeline;
		vk::PipelineLayout layout;
		bool bReady = false;
	} m_standby;
#endif

private:
	Info m_info;
	std::string m_name;
	WindowID m_window;

public:
	Pipeline(WindowID presenterWindow);
	~Pipeline();

public:
	bool create(Info info);
	void update();

private:
	bool create(vk::Pipeline& out_pipeline, vk::PipelineLayout& out_layout);
	void destroy(vk::Pipeline& out_pipeline, vk::PipelineLayout& out_layout);
};
} // namespace le::gfx
