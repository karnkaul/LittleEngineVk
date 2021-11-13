#pragma once
#include <dens/entity.hpp>

namespace dens {
class registry;
}

namespace le::gui {
class TreeRoot;
}

namespace le::edi {
template <typename T>
struct Inspect {
	T& out_t;
	dens::registry& registry;
	dens::entity entity;

	T& get() const noexcept { return out_t; }
};
} // namespace le::edi
