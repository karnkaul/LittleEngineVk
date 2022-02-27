#pragma once
#include <dens/entity.hpp>

namespace dens {
class registry;
}

namespace le {
class AssetStore;
namespace gui {
class TreeRoot;
}
} // namespace le

namespace le::editor {
template <typename T>
struct Inspect {
	T& out_t;
	dens::registry& registry;
	dens::entity entity;
	AssetStore const& store;

	T& get() const noexcept { return out_t; }
};
} // namespace le::editor
