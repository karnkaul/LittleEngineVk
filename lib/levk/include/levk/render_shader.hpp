#pragma once
#include <cstddef>
#include <span>

namespace levk {
enum class DescriptorSetType : int {
	eView = 0,
	eInstances = 1,
	eMaterial = 2,
	eJoints = 3,
	eCOUNT_,
};

/// \brief View of a SPIR-V shader for a pipeline.
using RenderShader = std::span<std::byte const>;
} // namespace levk
