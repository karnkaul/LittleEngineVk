#pragma once
#include <core/std_types.hpp>
#include <ktl/enum_flags/enum_flags.hpp>

namespace le::graphics {
enum class PipelineFlag { eDepthTest, eDepthWrite, eAlphaBlend, eWireframe };
using PipelineFlags = ktl::enum_flags<PipelineFlag, u8>;

using PFlag = PipelineFlag;
using PFlags = PipelineFlags;

constexpr PFlags pflags_all = PFlags(PFlag::eDepthTest, PFlag::eDepthWrite, PFlag::eAlphaBlend);

static_assert(pflags_all.count() == 3, "Invariant violated");
} // namespace le::graphics
