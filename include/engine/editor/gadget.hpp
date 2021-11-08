#pragma once
#include <core/utils/vbase.hpp>
#include <dens/registry.hpp>

namespace le::gui {
class TreeRoot;
}

namespace le::edi {
struct Gadget : utils::VBase {
	virtual bool operator()(dens::entity entity, dens::registry& registry) = 0;
};
struct GuiGadget : utils::VBase {
	virtual bool operator()(gui::TreeRoot& root) = 0;
};
} // namespace le::edi
