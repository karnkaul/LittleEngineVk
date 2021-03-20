#pragma once
#include <dumb_ecf/registry.hpp>
#include <engine/ibase.hpp>
#include <engine/scene_node.hpp>

namespace le::edi {
struct Gadget : IBase {
	virtual bool operator()(SceneNode& node, decf::registry_t& registry) = 0;
};
} // namespace le::edi
