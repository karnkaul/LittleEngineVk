#pragma once
#include <le/vfs/uri.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
struct PipelineFormat {
	vk::Format colour{};
	vk::Format depth{};
};

struct PipelineState {
	vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
	vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
	vk::CompareOp depth_compare{vk::CompareOp::eLess};
	vk::Bool32 depth_test_write{1};
	float line_width{1.0f};
};
} // namespace le::graphics
