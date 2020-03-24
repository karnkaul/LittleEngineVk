#pragma once
#include <string>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"
#include "engine/window/common.hpp"
#include "gfx/common.hpp"

namespace le::gfx
{
class Pipeline final
{
public:
	static std::string const s_tName;

private:
	PipelineData m_data;
	std::string m_name;
	vk::Pipeline m_pipeline;
	vk::PipelineLayout m_layout;
	WindowID m_window;

public:
	Pipeline(WindowID presenterWindow);
	~Pipeline();

public:
	bool create(PipelineData data);
	bool recreate();
	void destroy();
	void update();

public:
	vk::Pipeline pipeline() const;
	vk::PipelineLayout layout() const;
};
} // namespace le::gfx
