#pragma once
#include <string>
#include "core/std_types.hpp"
#include "engine/window/common.hpp"
#include "gfx/vulkan.hpp"

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

private:
	Info m_info;
	std::string m_name;
	vk::Pipeline m_pipeline;
	vk::PipelineLayout m_layout;
	WindowID m_window;

public:
	Pipeline(WindowID presenterWindow);
	~Pipeline();

public:
	bool create(Info info);
	bool recreate();
	void destroy();
	void update();

public:
	vk::Pipeline pipeline() const;
	vk::PipelineLayout layout() const;
};
} // namespace le::gfx
