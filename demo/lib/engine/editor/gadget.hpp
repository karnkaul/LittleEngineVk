#pragma once
#include <dumb_ecf/registry.hpp>
#include <engine/ibase.hpp>
#include <engine/scene_node.hpp>

namespace le::edi {
struct Gadget : IBase {
	virtual bool show(SceneNode const& node, decf::registry_t const& registry) const = 0;
	virtual void operator()(SceneNode& node, decf::registry_t& registry) = 0;
};
} // namespace le::edi
