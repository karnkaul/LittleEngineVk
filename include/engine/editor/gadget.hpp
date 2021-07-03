#pragma once
#include <dumb_ecf/registry.hpp>
#include <engine/utils/vbase.hpp>

namespace le::edi {
struct Gadget : utils::VBase {
	virtual bool operator()(decf::entity_t entity, decf::registry_t& registry) = 0;
};
} // namespace le::edi
