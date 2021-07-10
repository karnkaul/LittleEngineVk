#pragma once
#include <core/utils/vbase.hpp>
#include <dumb_ecf/registry.hpp>

namespace le::edi {
struct Gadget : utils::VBase {
	virtual bool operator()(decf::entity entity, decf::registry& registry) = 0;
};
} // namespace le::edi
