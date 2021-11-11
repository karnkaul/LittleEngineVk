#pragma once
#include <dens/registry.hpp>

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

template <typename T>
	requires(std::is_base_of_v<gui::TreeRoot, T>)
struct Inspect<T> {
	T& out_t;

	T& get() const noexcept { return out_t; }
};
} // namespace le::edi
